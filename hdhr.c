#include <Python.h>
#include <pyerrors.h>

#include <hdhomerun.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include <stdlib.h>
#include <stdio.h>

// This is the constant defined in hdhomerun_discover.c .
#define HDHOMERUN_DISOCVER_MAX_SOCK_COUNT 16

static PyObject *hdhr_find_devices(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_tuner_status(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_tuner_vstatus(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_supported(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_channel_list(PyObject *self, PyObject *args, PyObject *keywds);

#ifndef hdhomerun_channel_entry_t

struct hdhomerun_channel_entry_t {
	struct hdhomerun_channel_entry_t *next;
	struct hdhomerun_channel_entry_t *prev;
	uint32_t frequency;
	uint16_t channel_number;
	char name[16];
};

#endif

static char module_docstring[] =
    "This module provides an interface to HDHomeRun network TV tuners.";
static char find_devices_docstring[] =
    "Poll devices.";
static char get_tuner_status_docstring[] =
    "Get status information for the current channel.";
static char get_tuner_vstatus_docstring[] =
    "Get status information for the current virtual-channel.";
static char hdhr_get_supported_docstring[] =
    "Get supported modulations, channelmaps, etc..";
static char hdhr_get_channel_list_docstring[] =
    "Get dictionary of channels and frequencies.";

static PyMethodDef module_methods[] = {
    {"find_devices", (PyCFunction)hdhr_find_devices, METH_VARARGS | METH_KEYWORDS, find_devices_docstring},
    {"get_tuner_status", (PyCFunction)hdhr_get_tuner_status, METH_VARARGS | METH_KEYWORDS, get_tuner_status_docstring},
    {"get_tuner_vstatus", (PyCFunction)hdhr_get_tuner_vstatus, METH_VARARGS | METH_KEYWORDS, get_tuner_vstatus_docstring},
    {"get_supported", (PyCFunction)hdhr_get_supported, METH_VARARGS | METH_KEYWORDS, hdhr_get_supported_docstring},
    {"get_channel_list", (PyCFunction)hdhr_get_channel_list, METH_VARARGS | METH_KEYWORDS, hdhr_get_channel_list_docstring},

    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inithdhr(void)
{
    PyObject *m;
    if ((m = Py_InitModule3("hdhr", module_methods, module_docstring)) == NULL)
        return;
}

static PyObject *hdhr_find_devices(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_ip = 0;
    unsigned char param_maxcount = 255;

    int ip = 0;
    int i;

    char device_ip_str[16];
    char device_id_str[16];
    struct in_addr ip_struct;

    struct hdhomerun_discover_device_t *result_list;
    int found;

    PyObject *nice_devices;
    PyObject *temp;

    // Parse arguments.
    //
    // We expect an optional IP, and an optional maximum number of found 
    // devices.

    static char *kwlist[] = {"ip", "max_count", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|s|b", kwlist, &param_ip, &param_maxcount))
        return NULL;

    // Convert the IP to an integer, if given.

    if(param_ip != 0 && (ip = inet_network(param_ip)) == -1)
    {
        PyErr_SetString(PyExc_ValueError, "IP address is not valid.");
        return NULL;
    }

    // Allocate space for device-information.

    result_list = (struct hdhomerun_discover_device_t *)calloc(param_maxcount, sizeof(struct hdhomerun_discover_device_t));

    if(result_list == 0)
    {
        PyErr_SetString(PyExc_MemoryError, "Could not preallocate memory for device-information.");
        return NULL;
    }

    // Scan for devices.

    found = hdhomerun_discover_find_devices_custom(
            ip, 
            HDHOMERUN_DEVICE_TYPE_TUNER, 
            HDHOMERUN_DEVICE_ID_WILDCARD, 
            result_list, 
            param_maxcount
        );

    if(found == -1)
    {
        PyErr_SetString(PyExc_RuntimeError, "There was a problem while searching for devices. There was probably a socket problem.");
        return NULL;
    }

    // Build the list of results.

    if((nice_devices = Py_BuildValue("[]")) == NULL)
        return NULL;

    i = 0;
    while(i < found)
    {
        ip = htonl(result_list[i].ip_addr);
        ip_struct.s_addr = ip;
        strcpy(device_ip_str, inet_ntoa(ip_struct));

        sprintf(device_id_str, "%X", result_list[i].device_id);

        if((temp = Py_BuildValue(
                    "{s:s:s:I:s:s:s:b}", 
                    "IP", device_ip_str, 
                    "Type", result_list[i].device_type, 
                    "ID", device_id_str, 
                    "TunerCount", result_list[i].tuner_count
                )) == NULL)
            return NULL;

        if(PyList_Append(nice_devices, temp) == -1)
            return NULL;

        i++;
    }

    free(result_list);

    return nice_devices;
}

static PyObject *hdhr_get_tuner_status(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter.

    static char *kwlist[] = {"param_id_or_ip", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &param_id_or_ip))
        return NULL;

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    struct hdhomerun_tuner_status_t status;
    int status_result;

    if((status_result = hdhomerun_device_get_tuner_status(hd, NULL, &status)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Communication with device failed for status request.");
        return NULL;
    }

    else if(status_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Device rejected get-tuner-status request.");
        return NULL;
    }

    PyObject *tuner_status;

    tuner_status = Py_BuildValue(
            "{s:s:s:s:s:O:s:O:s:O:s:I:s:I:s:I:s:I:s:I}", 
            "Channel",              status.channel, 
            "LockString",           status.lock_str, 

            "SignalPresent",        (status.signal_present ? Py_True : Py_False),
            "LockSupported",        (status.lock_supported ? Py_True : Py_False),
            "LockUnsupported",      (status.lock_unsupported ? Py_True : Py_False),

            "SignalStrength",       status.signal_strength, 
            "SignalNoiseQuality",   status.signal_to_noise_quality,
            "SymbolErrorQuality",   status.symbol_error_quality,
            "RawBitsPerSecond",     status.raw_bits_per_second,
            "PacketsPerSecond",     status.packets_per_second
        );

    hdhomerun_device_destroy(hd);

    if(tuner_status == NULL)
        return NULL;

    return tuner_status;
}

static PyObject *hdhr_get_tuner_vstatus(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter.

    static char *kwlist[] = {"param_id_or_ip", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &param_id_or_ip))
        return NULL;

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    struct hdhomerun_tuner_vstatus_t vstatus;
    int vstatus_result;

    if((vstatus_result = hdhomerun_device_get_tuner_vstatus(hd, NULL, &vstatus)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Communication with device failed for vstatus request.");
        return NULL;
    }

    else if(vstatus_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Device rejected get-tuner-vstatus request.");
        return NULL;
    }

    PyObject *tuner_vstatus;

    tuner_vstatus = Py_BuildValue(
            "{s:s:s:s:s:s:s:s:s:s:s:O:s:O:s:O}", 
            "VChannel",         vstatus.vchannel, 
            "Name",             vstatus.name, 
            "Auth",             vstatus.auth, 
            "CCI",              vstatus.cci,
            "CGMS",             vstatus.cgms,

            "NotSubscribed",    (vstatus.not_subscribed ? Py_True : Py_False),
            "NotAvailable",     (vstatus.not_available ? Py_True : Py_False),
            "CopyProtected",    (vstatus.copy_protected ? Py_True : Py_False)
        );

    hdhomerun_device_destroy(hd);

    if(tuner_vstatus == NULL)
        return NULL;

    return tuner_vstatus;
}

static PyObject *hdhr_get_supported(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_prefix = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and an 
    // optional prefix to identify a particular row.

    static char *kwlist[] = {"id_or_ip", "prefix", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|s", kwlist, &param_id_or_ip, &param_prefix))
        return NULL;

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    char *pstr;
    int get_supported_result;

    if((get_supported_result = hdhomerun_device_get_supported(hd, param_prefix, &pstr)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Get-supported request failed.");
        return NULL;
    }

    else if(get_supported_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Device rejected get-supported request.");
        return NULL;
    }

    hdhomerun_device_destroy(hd);

    return Py_BuildValue("s", pstr);
}

static PyObject *hdhr_get_channel_list(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_channelmap = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and an 
    // optional prefix to identify a particular row.

    static char *kwlist[] = {"id_or_ip", "channelmap", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &param_id_or_ip, &param_channelmap))
        return NULL;

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    struct hdhomerun_channel_list_t *channel_list;

    PyObject *nice_dict;

    if((nice_dict = Py_BuildValue("{}")) == NULL)
        return NULL;

    // There was an error or there were no channels.
    if((channel_list = hdhomerun_channel_list_create(param_channelmap)) == NULL)
        return nice_dict;

    // Move through each channel and set them on the dictionary.

    struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);

    PyObject *entry_key;
    PyObject *entry_value;

    while(entry)
    {
        if((entry_key = Py_BuildValue("I", entry->channel_number)) == NULL)
        {
            hdhomerun_channel_list_destroy(channel_list);
            return NULL;
        }

        if((entry_value = Py_BuildValue("I", entry->frequency)) == NULL)
        {
            hdhomerun_channel_list_destroy(channel_list);
            return NULL;
        }

        if(PyDict_SetItem(nice_dict, entry_key, entry_value) == -1)
        {
            hdhomerun_channel_list_destroy(channel_list);
            return NULL;
        }

        entry = hdhomerun_channel_list_next(channel_list, entry);
    }

    hdhomerun_channel_list_destroy(channel_list);

    return nice_dict;
}

