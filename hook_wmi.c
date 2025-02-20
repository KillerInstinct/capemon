#include "log.h"
#include "misc.h"
#include "config.h"

static int g_wmi_disk = 0;
static int g_wmi_computer = 0;
static int g_wmi_ram = 0;
static int g_wmi_videocard = 0;


HOOKDEF(HRESULT, WINAPI, WMI_Get,
	PVOID		_this,
	LPCWSTR		wszName,
	LONG		lFlags,
	VARIANT*	pVal,
	LONG*		pType,
	LONG*		plFlavor
) {
	HRESULT ret;
	ret = Old_WMI_Get(_this, wszName, lFlags, pVal, pType, plFlavor);
	LOQ_hresult("system", "un", "Name", wszName, "Value", pVal);

	if (!g_config.no_stealth && wszName) {
		if (g_wmi_disk && pVal) {
			// Spoofery for Win32_LogicalDisk
			if (pVal->vt == VT_BSTR && !wcsicmp(wszName, L"Size")){
				unsigned long long lSize = wcstoull(pVal->bstrVal, NULL, 10);
				if (lSize < SPOOFED_DISK_SIZE - RECOVERY_PARTITION_SIZE) {
					wchar_t newSize[16];
					memset(newSize, 0x0, sizeof(newSize));
					swprintf_s(newSize, sizeof(newSize), L"%llu", SPOOFED_DISK_SIZE - RECOVERY_PARTITION_SIZE);
					SysFreeString(pVal->bstrVal);
					pVal->bstrVal = SysAllocString(newSize);
				}
			}
		}
		else if (g_wmi_computer && pVal) {
			// Spoofery for Win32_ComputerSystem
			if (pVal->vt == VT_BSTR) {
				if (!wcsicmp(wszName, L"TotalPhysicalMemory")) {
					unsigned long actualMemory = wcstoul(pVal->bstrVal, NULL, 10);
					unsigned long spoofedMemory = SPOOFED_RAM - SPOOFED_RAM_RESERVED;
					if (actualMemory < spoofedMemory) {
						wchar_t wszMemory[16];
						memset(wszMemory, 0x0, sizeof(wszMemory));
						swprintf_s(wszMemory, sizeof(wszMemory), L"%lu", spoofedMemory);
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(wszMemory);
					}
				}
				else if (!wcsicmp(wszName, L"Manufacturer")) {
					SysFreeString(pVal->bstrVal);
					pVal->bstrVal = SysAllocString(L"Dell Inc.");
				}
			}
		}
		else if (g_wmi_ram && pVal) {
			// Spoofery for Win32_PhysicalMemory
			if (pVal->vt == VT_BSTR) {
				if (!wcsicmp(wszName, L"Capacity")) {
					unsigned long actualMemory = wcstoul(pVal->bstrVal, NULL, 10);
					unsigned long spoofedMemory = SPOOFED_RAM - SPOOFED_RAM_RESERVED;
					if (actualMemory < spoofedMemory) {
						wchar_t wszMemory[16];
						memset(wszMemory, 0x0, sizeof(wszMemory));
						swprintf_s(wszMemory, sizeof(wszMemory), L"%lu", spoofedMemory);
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(wszMemory);
					}
				}
				else if (!wcsicmp(wszName, L"Manufacturer")) {
					SysFreeString(pVal->bstrVal);
					pVal->bstrVal = SysAllocString(L"Corsair");
				}
			}
		}
		else if (g_wmi_videocard && pVal) {
			// Spoofery for Win32_VideoController
			if (pVal->vt == VT_BSTR) {
				if (!wcsicmp(wszName, L"Caption")) {
					if (!wcsicmp(pVal->bstrVal, L"Microsoft Basic Display Adapter") ||
						!wcsicmp(pVal->bstrVal, L"Standard VGA Graphics Adapter")) {
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(SPOOFED_GPU_NAME);
					}
				}
				else if (!wcsicmp(wszName, L"Description")) {
					if (!wcsicmp(pVal->bstrVal, L"Microsoft Basic Display Adapter") ||
						!wcsicmp(pVal->bstrVal, L"Standard VGA Graphics Adapter")) {
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(SPOOFED_GPU_NAME);
					}
				}
				else if (!wcsicmp(wszName, L"Name")) {
					if (!wcsicmp(pVal->bstrVal, L"Microsoft Basic Display Adapter") ||
						!wcsicmp(pVal->bstrVal, L"Standard VGA Graphics Adapter")) {
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(SPOOFED_GPU_NAME);
					}
				}
				else if (!wcsicmp(wszName, L"VideoProcessor")) {
					if (wcsstr(pVal->bstrVal, L"SeaBIOS")) {
						SysFreeString(pVal->bstrVal);
						pVal->bstrVal = SysAllocString(SPOOFED_GPU_NAME);
					}
				}
			}
			else if (pVal->vt == VT_NULL) {
				// If these fields are empty, populate them
				if (!wcsicmp(wszName, L"VideoProcessor")) {
					pVal->vt = VT_BSTR;
					pVal->bstrVal = SysAllocString(SPOOFED_GPU_NAME);
				}
			}
		}
		else if (pVal && pVal->vt == VT_I4) {
			// General spoofery on int variants
			if (!wcsicmp(wszName, L"NumberOfCores")) {
				if (pVal->lVal < SPOOFED_CPU_CORE_NUM) {
					pVal->lVal = SPOOFED_CPU_CORE_NUM;
				}
			}
			else if (!wcsicmp(wszName, L"AdapterRAM")) {
				if (pVal->lVal < SPOOFED_GPU_RAM) {
					pVal->lVal = SPOOFED_GPU_RAM;
				}
			}
			else if (!wcsicmp(wszName, L"MaxRefreshRate")) {
				if (pVal->lVal < 75) {
					pVal->lVal = 75;
				}
			}
		}
		else if (pVal && pVal->vt == VT_NULL) {
			// General spoofery on NULL variants
			if (!wcsicmp(wszName, L"AdapterRAM")) {
				pVal->vt = VT_I4;
				pVal->lVal = SPOOFED_GPU_RAM;
			}
			else if (!wcsicmp(wszName, L"MaxRefreshRate")) {
				pVal->vt = VT_I4;
				pVal->lVal = 75;
			}
		}

	}

	return ret;
}

HOOKDEF_NOTAIL(WINAPI, WMI_ExecQuery,
	PVOID		_this,
	const BSTR	strQueryLanguage,
	const BSTR	strQuery,
	LONG		lFlags,
	PVOID		pCtx,
	PVOID*		ppEnum
) {
	HRESULT ret = 0;

	// Reset our tracking variables when we enter a new ExecQuery
	g_wmi_disk = 0;
	g_wmi_computer = 0;
	g_wmi_ram = 0;
	g_wmi_videocard = 0;

	LOQ_hresult("system", "u", "Query", strQuery);

	if (!g_config.no_stealth && strQuery) {
		// Look for an interesting WMI query
		if (!_wcsnicmp(strQuery, L"SELECT ", 7)) {
			if (wcsistr(strQuery, L" FROM Win32_LogicalDisk")) {
				g_wmi_disk = 1;
			}
			else if (wcsistr(strQuery, L" FROM Win32_ComputerSystem")) {
				g_wmi_computer = 1;
			}
			else if (wcsistr(strQuery, L" FROM Win32_PhysicalMemory")) {
				g_wmi_ram = 1;
			}
			else if (wcsistr(strQuery, L" FROM Win32_VideoController")) {
				g_wmi_videocard = 1;
			}
		}
	}
	return 0;
}

HOOKDEF_NOTAIL(WINAPI, WMI_ExecQueryAsync,
	PVOID		_this,
	const BSTR	strQueryLanguage,
	const BSTR	strQuery,
	long		lFlags,
	PVOID		pCtx,
	PVOID		pResponseHandler
) {
	HRESULT ret = 0;
	LOQ_hresult("system", "u", "Query", strQuery);
	return 0;
}

HOOKDEF_NOTAIL(WINAPI, WMI_ExecMethod,
	PVOID		_this,
	const BSTR	strObjectPath,
	const BSTR	strMethodName,
	long		lFlags,
	PVOID		pCtx,
	PVOID		pInParams,
	PVOID*		ppOutParams,
	PVOID*		ppCallResult
) {
	HRESULT ret = 0;
	LOQ_hresult("system", "uu", "ObjectPath", strObjectPath, "MethodName", strMethodName);
	return 0;
}

HOOKDEF_NOTAIL(WINAPI, WMI_ExecMethodAsync,
	PVOID		_this,
	const BSTR	strObjectPath,
	const BSTR	strMethodName,
	long		lFlags,
	PVOID		pCtx,
	PVOID		pInParams,
	PVOID		pResponseHandler
) {
	HRESULT ret = 0;
	LOQ_hresult("system", "uu", "ObjectPath", strObjectPath, "MethodName", strMethodName);
	return 0;
}

HOOKDEF_NOTAIL(WINAPI, WMI_GetObject,
	PVOID           _this,
	const BSTR      strObjectPath,
	long            lFlags,
	PVOID           pCtx,
	PVOID*          ppObject,
	PVOID*          ppCallResult
) {
	HRESULT ret = 0;
	if (strObjectPath && SysStringLen(strObjectPath) > 0)
		LOQ_hresult("system", "u", "ObjectPath", strObjectPath);
	else
		LOQ_hresult("system", "u", "ObjectPath", L"[NULL or Empty]");
	return 0;
}

HOOKDEF_NOTAIL(WINAPI, WMI_GetObjectAsync,
	PVOID		_this,
	const BSTR	strObjectPath,
	long		lFlags,
	PVOID		pCtx,
	PVOID		pResultHandler
) {
	HRESULT ret = 0;
	if (strObjectPath && SysStringLen(strObjectPath) > 0)
		LOQ_hresult("system", "u", "ObjectPath", strObjectPath);
	else
		LOQ_hresult("system", "u", "ObjectPath", L"[NULL or Empty]");
	return 0;
}