#include <Python.h>
#include <pyerrors.h>

#include <hdhomerun.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>

static PyObject *hdhr_find_devices(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_tuner_status(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_tuner_vstatus(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_supported(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_get_channel_list(PyObject *self, PyObject *args, PyObject *keywds);
//static PyObject *hdhr_acquire_lockkey(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_set_vchannel(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_set_target(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_wait_for_lock(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_clear_target(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_scan_channels(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *hdhr_scan_channels(PyObject *self, PyObject *args, PyObject *keywds);

#ifndef hdhomerun_channel_entry_t

// We've had issues with this.

struct hdhomerun_channel_entry_t {
	struct hdhomerun_channel_entry_t *next;
	struct hdhomerun_channel_entry_t *prev;
	uint32_t frequency;
	uint16_t channel_number;
	char name[16];
};

#endif

#ifndef hdhomerun_device_t

struct hdhomerun_device_t {
	struct hdhomerun_control_sock_t *cs;
	struct hdhomerun_video_sock_t *vs;
	struct hdhomerun_debug_t *dbg;
	struct hdhomerun_channelscan_t *scan;
	uint32_t multicast_ip;
	uint16_t multicast_port;
	uint32_t device_id;
	unsigned int tuner;
	uint32_t lockkey;
	char name[32];
	char model[32];
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
    "Get dictionary of calculated channels and frequencies.";
//static char hdhr_acquire_lockkey_docstring[] =
//    "Acquire lock-key for channel changing.";
static char hdhr_set_vchannel_docstring[] =
    "Set current vchannel.";
static char hdhr_set_target_docstring[] =
    "Set current target (start streaming).";
static char hdhr_wait_for_lock_docstring[] =
    "Wait for the new channel to lock-in.";
static char hdhr_clear_target_docstring[] =
    "Clear the current target (stop streaming).";
static char hdhr_scan_channels_docstring[] =
    "Scan for channels.";

static PyMethodDef module_methods[] = {
    {"find_devices",      (PyCFunction)hdhr_find_devices,      METH_VARARGS | METH_KEYWORDS, find_devices_docstring},
    {"get_tuner_status",  (PyCFunction)hdhr_get_tuner_status,  METH_VARARGS | METH_KEYWORDS, get_tuner_status_docstring},
    {"get_tuner_vstatus", (PyCFunction)hdhr_get_tuner_vstatus, METH_VARARGS | METH_KEYWORDS, get_tuner_vstatus_docstring},
    {"get_supported",     (PyCFunction)hdhr_get_supported,     METH_VARARGS | METH_KEYWORDS, hdhr_get_supported_docstring},
    {"get_channel_list",  (PyCFunction)hdhr_get_channel_list,  METH_VARARGS | METH_KEYWORDS, hdhr_get_channel_list_docstring},
//    {"acquire_lockkey",   (PyCFunction)hdhr_acquire_lockkey,   METH_VARARGS | METH_KEYWORDS, hdhr_acquire_lockkey_docstring},
    {"set_vchannel",      (PyCFunction)hdhr_set_vchannel,      METH_VARARGS | METH_KEYWORDS, hdhr_set_vchannel_docstring},
    {"set_target",        (PyCFunction)hdhr_set_target,        METH_VARARGS | METH_KEYWORDS, hdhr_set_target_docstring},
    {"wait_for_lock",     (PyCFunction)hdhr_wait_for_lock,     METH_VARARGS | METH_KEYWORDS, hdhr_wait_for_lock_docstring},
    {"clear_target",      (PyCFunction)hdhr_clear_target,      METH_VARARGS | METH_KEYWORDS, hdhr_clear_target_docstring},
    {"scan_channels",     (PyCFunction)hdhr_scan_channels,     METH_VARARGS | METH_KEYWORDS, hdhr_scan_channels_docstring},

    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inithdhr(void)
{
    PyObject *m;
    if ((m = Py_InitModule3("hdhr", module_methods, module_docstring)) == NULL)
        return;
}

// Discover devices on the local network.
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

// Get current status of tuner (low-level).
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

// Get current status of tuner (high-level).
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

// Get supported modulations, channelmaps, etc..
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

// Get dictionary of channel-numbers and frequencies. This is calculated, not 
// discovered.
static PyObject *hdhr_get_channel_list(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_channelmap = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and the
    // name of a channelmap.

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
    {
        hdhomerun_device_destroy(hd);

        return NULL;
    }

    // There was an error or there were no channels.
    if((channel_list = hdhomerun_channel_list_create(param_channelmap)) == NULL)
    {
        hdhomerun_device_destroy(hd);

        return nice_dict;
    }

    // Move through each channel and set them on the dictionary.

    struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);

    PyObject *entry_key;
    PyObject *entry_value;

    while(entry)
    {
        if((entry_key = Py_BuildValue("I", entry->channel_number)) == NULL)
        {
            hdhomerun_channel_list_destroy(channel_list);
            hdhomerun_device_destroy(hd);

            return NULL;
        }

        if((entry_value = Py_BuildValue("I", entry->frequency)) == NULL)
        {
            hdhomerun_channel_list_destroy(channel_list);
            hdhomerun_device_destroy(hd);

            return NULL;
        }

        if(PyDict_SetItem(nice_dict, entry_key, entry_value) == -1)
        {
            hdhomerun_channel_list_destroy(channel_list);
            hdhomerun_device_destroy(hd);

            return NULL;
        }

        entry = hdhomerun_channel_list_next(channel_list, entry);
    }

    hdhomerun_channel_list_destroy(channel_list);

    return nice_dict;
}

/* Hasn't been necessary to change channels.
// Acquire a lock-key to use to change channels.
static PyObject *hdhr_acquire_lockkey(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and an 
    // optional prefix to identify a particular row.

    static char *kwlist[] = {"id_or_ip", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &param_id_or_ip))
        return NULL;

    struct hdhomerun_device_t *hd;

	if((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for lockkey.");
        return NULL;
	}

    int lockkey_result;
    char *error;

    if((lockkey_result = hdhomerun_device_tuner_lockkey_request(hd, &error)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Request to acquire lock-key failed.");
        return NULL;
    }

    else if(lockkey_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Device rejected lock-key-acquire request.");
        return NULL;
    }

    unsigned int lockkey = hd->lockkey;

    hdhomerun_device_tuner_lockkey_release(hd);
    hdhomerun_device_destroy(hd);

    PyObject *nice_lockkey;
   
    if((nice_lockkey = Py_BuildValue("I", &lockkey)) == NULL)
        return NULL;

    return nice_lockkey;
}
*/

// Request to set the vchannel.
static PyObject *hdhr_set_vchannel(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_vchannel = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and a
    // vchannel.

    static char *kwlist[] = {"id_or_ip", "vchannel", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &param_id_or_ip, &param_vchannel))
        return NULL;

    struct hdhomerun_device_t *hd;

	if((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for vchannel change.");
        return NULL;
	}

    int vchannel_result;
    if((vchannel_result = hdhomerun_device_set_tuner_vchannel(hd, param_vchannel)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Request to set vchannel failed.");
        return NULL;
    }

    else if(vchannel_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Request to set vchannel was rejected.");
        return NULL;
    }

    hdhomerun_device_destroy(hd);

    return Py_BuildValue("");
}

// Request to set the stream target.
static PyObject *hdhr_set_target(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_target = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and a
    // "<IP>:<port>" for the outgoing UDP feed.

    static char *kwlist[] = {"id_or_ip", "target", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &param_id_or_ip, &param_target))
        return NULL;

    struct hdhomerun_device_t *hd;

	if((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for target change.");
        return NULL;
	}

    int target_result;
    if((target_result = hdhomerun_device_set_tuner_target(hd, param_target)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Request to set target failed.");
        return NULL;
    }

    else if(target_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Request to set target was rejected.");
        return NULL;
    }

    hdhomerun_device_destroy(hd);

    return Py_BuildValue("");
}

// Request to check the status of a channel-change.
static PyObject *hdhr_wait_for_lock(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and a
    // "<IP>:<port>" for the outgoing UDP feed.

    static char *kwlist[] = {"id_or_ip", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &param_id_or_ip))
        return NULL;

    struct hdhomerun_device_t *hd;

	if((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for the wait-for-lock request.");
        return NULL;
	}

    struct hdhomerun_tuner_status_t status;

    int hold_result;
    if((hold_result = hdhomerun_device_wait_for_lock(hd, &status)) == -1)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_IOError, "Request to wait for the lock failed.");
        return NULL;
    }

    else if(hold_result == 0)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Request to wait for the lock was rejected.");
        return NULL;
    }

    PyObject *have_signal = Py_BuildValue("O", (status.signal_present ? Py_True : Py_False));

    hdhomerun_device_destroy(hd);

    return have_signal;
}

// Request to stop streaming.
static PyObject *hdhr_clear_target(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and a
    // "<IP>:<port>" for the outgoing UDP feed.

    static char *kwlist[] = {"id_or_ip", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &param_id_or_ip))
        return NULL;

    struct hdhomerun_device_t *hd;

	if((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for the wait-for-lock request.");
        return NULL;
	}

    hdhomerun_device_stream_stop(hd);

    hdhomerun_device_destroy(hd);

    return Py_BuildValue("");
}

// Scan channels.
static PyObject *hdhr_scan_channels(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *param_id_or_ip = 0;
    char *param_channelmap = 0;
    PyObject *param_callback = 0;

    char message[200];

    // Parse arguments.
    //
    // We expect an IP, ID, or "<ID>-<tuner>" as the first parameter, and the
    // name of a channelmap.

    static char *kwlist[] = {"id_or_ip", "channelmap", "status_callback", NULL};

    char have_callback = 0;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|O", kwlist, &param_id_or_ip, &param_channelmap, &param_callback))
        return NULL;

    else if(param_callback != Py_BuildValue(""))
    {
        if(!PyCallable_Check(param_callback))
        {
            PyErr_SetString(PyExc_RuntimeError, "The callback must be callable, if provided.");
            return NULL;
        }

        have_callback = 1;
    }

    struct hdhomerun_device_t *hd;

	if ((hd = hdhomerun_device_create_from_str(param_id_or_ip, NULL)) == NULL) 
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not create device structure for channel-scan.");
        return NULL;
	}
	
    struct hdhomerun_channelscan_t *scan;
    
    if((scan = channelscan_create(hd, param_channelmap)) == NULL)
    {
        hdhomerun_device_destroy(hd);

        PyErr_SetString(PyExc_RuntimeError, "Could not create channel-scan object.");
        return NULL;
    }

    struct hdhomerun_channelscan_result_t result;
    int channel_result;
    int i = 0;
    PyObject *nice_list;
    
    if((nice_list = Py_BuildValue("[]")) == NULL)
    {
        channelscan_destroy(scan);
        hdhomerun_device_destroy(hd);

        return NULL;
    }
    
    PyObject *nice_channel_info;
    PyObject *callback_payload_dict;
    PyObject *callback_payload_tuple;
    PyObject *callback_result;
    PyObject *nice_program_list;
    PyObject *nice_program;
    int progress;
    char scan_terminated = 0;
    int j;

    while((channel_result = channelscan_advance(scan, &result)) != 0)
    {
        if((channel_result = channelscan_detect(scan, &result)) == -1)
        {
            channelscan_destroy(scan);
            hdhomerun_device_destroy(hd);

            sprintf(message, "There was an error while detecting on channel with index (%d).", i);
        
            PyErr_SetString(PyExc_RuntimeError, message);
            return NULL;
        }
        
        else if(channel_result == 0)
        {
            i++;
            continue;
        }

        if((nice_program_list = Py_BuildValue("[]")) == NULL)
        {
            channelscan_destroy(scan);
            hdhomerun_device_destroy(hd);

            return NULL;
        }

        j = 0;
        while(j < result.program_count)
        {
/*
struct hdhomerun_channelscan_program_t {
	char program_str[64];
	uint16_t program_number;
	uint16_t virtual_major;
	uint16_t virtual_minor;
	uint16_t type;
	char name[32];
};
*/
            if((nice_program = Py_BuildValue("s:s:s:b:s:b:s:b:s:b:s:s",
                    "Descriptor",   result.programs[j].program_str,
                    "Number",       result.programs[j].program_number,
                    "VirtualMajor", result.programs[j].virtual_major,
                    "VirtualMinor", result.programs[j].virtual_minor,
                    "Type",         result.programs[j].type,
                    "Name",         result.programs[j].name
                )) == NULL)
            {
                channelscan_destroy(scan);
                hdhomerun_device_destroy(hd);

                return NULL;
            }

            if(PyList_Append(nice_program_list, nice_program) == -1)
            {
                channelscan_destroy(scan);
                hdhomerun_device_destroy(hd);

                return NULL;
            }
        
            j++;
        }
        
        if((nice_channel_info = Py_BuildValue("s:s:s:I:s:I:s:i:s:O:s:b:s:O",
                "Descriptor",                   result.channel_str,
                "ChannelMap",                   result.channelmap,
                "Frequency",                    result.frequency,
                "ProgramCount",                 result.program_count,
                "TransportStreamIdDetected",    (result.transport_stream_id_detected ? Py_True : Py_False),
                "TransportStreamId",            result.transport_stream_id,
                "Programs",                     nice_program_list
                )) == NULL)
        {
            channelscan_destroy(scan);
            hdhomerun_device_destroy(hd);

            return NULL;
        }
        
        if(PyList_Append(nice_list, nice_channel_info) == -1)
        {
            channelscan_destroy(scan);
            hdhomerun_device_destroy(hd);

            return NULL;
        }
        
        if(have_callback)
        {
            progress = channelscan_get_progress(scan);

            if((callback_payload_tuple = Py_BuildValue("(O:b)",
                        nice_channel_info,
                        progress
                    )) == NULL)
            {
                channelscan_destroy(scan);
                hdhomerun_device_destroy(hd);

                return NULL;
            }
            
            if((callback_payload_dict = Py_BuildValue("{s:O:s:b}",
                        "channel_info",  nice_channel_info,
                        "scan_progress", progress
                    )) == NULL)
            {
                channelscan_destroy(scan);
                hdhomerun_device_destroy(hd);

                return NULL;
            }
            
            if((callback_result = PyObject_Call(param_callback, callback_payload_tuple, NULL)) == NULL)
            {
                channelscan_destroy(scan);
                hdhomerun_device_destroy(hd);

                return NULL;
            }

            if(!PyObject_IsTrue(callback_result))
            {
                scan_terminated = 1;
                break;
            }
        }
        
        i++;
    }

    channelscan_destroy(scan);
    hdhomerun_device_destroy(hd);

    if(scan_terminated)
        return Py_BuildValue("");
/*
struct hdhomerun_channelscan_result_t {
	char channel_str[64];
	uint32_t channelmap;
	uint32_t frequency;
	struct hdhomerun_tuner_status_t status;
	int program_count;
	struct hdhomerun_channelscan_program_t programs[HDHOMERUN_CHANNELSCAN_MAX_PROGRAM_COUNT];
	bool_t transport_stream_id_detected;
	uint16_t transport_stream_id;
};
*/

    return nice_list;
}

