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
static PyObject *hdhr_get_supported(PyObject *self, PyObject *args, PyObject *keywds);

static char module_docstring[] =
    "This module provides an interface to HDHomeRun network TV tuners.";
static char find_devices_docstring[] =
    "Poll devices.";
static char get_tuner_status_docstring[] =
    "Get status information for tuner.";
static char hdhr_get_supported_docstring[] =
    "Get supported modulations, channelmaps, etc..";

static PyMethodDef module_methods[] = {
    {"find_devices", (PyCFunction)hdhr_find_devices, METH_VARARGS | METH_KEYWORDS, find_devices_docstring},
    {"get_tuner_status", (PyCFunction)hdhr_get_tuner_status, METH_VARARGS | METH_KEYWORDS, get_tuner_status_docstring},
    {"get_supported", (PyCFunction)hdhr_get_supported, METH_VARARGS | METH_KEYWORDS, hdhr_get_supported_docstring},

    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inithdhr(void)
{
    PyObject *m = Py_InitModule3("hdhr", module_methods, module_docstring);
    if (m == NULL)
        return;
}

void dump_found(struct hdhomerun_local_ip_info_t found[], int num_found)
{
    char ip_raw[16];
    char subnet_raw[16];
    int ip;
    int subnet;
    struct in_addr ip_struct;

    int i;

    i = 0;
    while(i < num_found)
    {
        ip = htonl(found[i].ip_addr);
        ip_struct.s_addr = ip;
        strcpy(ip_raw, inet_ntoa(ip_struct));

        subnet = htonl(found[i].subnet_mask);
        ip_struct.s_addr = subnet;
        strcpy(subnet_raw, inet_ntoa(ip_struct));

        printf("Found(%d): IP= (%s) MASK= (%s)\n", i, ip_raw, subnet_raw);

        i++;
    }
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

    static char *kwlist[] = {"ip", "max_count", NULL};

    // Parse arguments.

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

    i = 0;
    nice_devices = Py_BuildValue("[]");
    while(i < found)
    {
        ip = htonl(result_list[i].ip_addr);
        ip_struct.s_addr = ip;
        strcpy(device_ip_str, inet_ntoa(ip_struct));

        sprintf(device_id_str, "%X", result_list[i].device_id);

        temp = Py_BuildValue(
                "{s:s:s:I:s:s:s:b}", 
                "IP", device_ip_str, 
                "Type", result_list[i].device_type, 
                "ID", device_id_str, 
                "TunerCount", result_list[i].tuner_count
            );

        if(PyList_Append(nice_devices, temp) == -1)
            return NULL;

        i++;
    }

    free(result_list);

    return nice_devices;
}

static PyObject *hdhr_get_tuner_status(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_ip = 0;
    unsigned char param_tuner_uc = 0;
    char param_tuner[4];

    int status_result;
    struct hdhomerun_tuner_status_t status;

    PyObject *temp;

    static char *kwlist[] = {"ip", "tuner", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sb", kwlist, &param_ip, &param_tuner_uc))
        return NULL;

    sprintf(param_tuner, "%u", param_tuner_uc);

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    if(hdhomerun_device_set_tuner_from_str(hd, param_tuner) == -1)
    {
        PyErr_SetString(PyExc_IOError, "Could not set tuner. It must be an unsigned integer from 0 to 255.");
        return NULL;
    }

    if((status_result = hdhomerun_device_get_tuner_status(hd, NULL, &status)) == -1)
    {
        PyErr_SetString(PyExc_IOError, "Communication with device failed.");
        return NULL;
    }

    else if(status_result == 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Device rejected get-tuner-status request.");
        return NULL;
    }

    temp = Py_BuildValue(
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

    return temp;
}

static PyObject *hdhr_get_supported(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_ip = 0;
    char *param_prefix = 0;

    static char *kwlist[] = {"ip", "prefix", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|s", kwlist, &param_ip, &param_prefix))
        return NULL;

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure.");
        return NULL;
	}

    char *pstr;
    int get_supported_result;

    if((get_supported_result = hdhomerun_device_get_supported(hd, param_prefix, &pstr)) == -1)
    {
        PyErr_SetString(PyExc_IOError, "Get-supported request failed.");
        return NULL;
    }

    else if(get_supported_result == 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Device rejected get-supported request.");
        return NULL;
    }

    hdhomerun_device_destroy(hd);

    return Py_BuildValue("s", pstr);
}

