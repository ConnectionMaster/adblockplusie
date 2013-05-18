#include "PluginStdAfx.h"

#include "Plugin.h"
#include "../build/AdblockPlus_i.c"

#include "PluginClass.h"
#include "PluginClient.h"
#include "PluginSystem.h"
#include "PluginSettings.h"
#include "PluginDictionary.h"
#include "PluginMimeFilterClient.h"
#include "Msiquery.h"

#ifdef SUPPORT_FILTER
#include "PluginFilter.h"
#endif
#ifdef SUPPORT_CONFIG
#include "PluginConfig.h"
#endif


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_PluginClass, CPluginClass)
END_OBJECT_MAP()

//Dll Entry Point
BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
  switch( fdwReason )
  {
  case DLL_PROCESS_ATTACH:
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);
    _tcslwr_s(szFilename);

    if (_tcsstr(szFilename, _T("explorer.exe")))
    {
      return FALSE;
    }

    _Module.Init(ObjectMap, _Module.GetModuleInstance(), &LIBID_PluginLib);
    break;

  case DLL_THREAD_ATTACH:
    // thread-specific initialization.
    break;

  case DLL_THREAD_DETACH:
    // thread-specific cleanup.
    break;

  case DLL_PROCESS_DETACH:
    // any necessary cleanup.
    break;
  }

  return TRUE;
}


STDAPI DllCanUnloadNow(void)
{
  LONG count = _Module.GetLockCount();
  if (_Module.GetLockCount() == 0)
  {
    if (CPluginSettings::s_instance != NULL)
    {
      delete CPluginSettings::s_instance;
    }


    if (CPluginSystem::s_instance != NULL)
    {
      delete CPluginSystem::s_instance;
    }

    if (CPluginClass::s_mimeFilter != NULL)
    {
      CPluginClass::s_mimeFilter->Unregister();
      CPluginClass::s_mimeFilter = NULL;
    }

    _CrtDumpMemoryLeaks();
  }
  return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
  return _Module.RegisterServer(TRUE);
}

STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer(TRUE);
}

void InitPlugin(bool isInstall)
{
  CPluginSystem* system = CPluginSystem::GetInstance();

  CPluginSettings* settings = CPluginSettings::GetInstance();

  settings->SetMainProcessId();
  settings->EraseTab();

  settings->Remove(SETTING_PLUGIN_SELFTEST);
  settings->SetValue(SETTING_PLUGIN_INFO_PANEL, isInstall ? 1 : 2);


  settings->Write();

  if (isInstall)
  {
    DEBUG_GENERAL(
      L"================================================================================\nINSTALLER " +
      CString(IEPLUGIN_VERSION) +
      L"\n================================================================================")
  }
  else
  {
    DEBUG_GENERAL(
      L"================================================================================\nUPDATER " +
      CString(IEPLUGIN_VERSION) + L" (UPDATED FROM " + settings->GetString(SETTING_PLUGIN_VERSION) + L")"
      L"\n================================================================================")
  }

  // Create default filters
#ifdef SUPPORT_FILTER
  //    DEBUG_GENERAL(L"*** Generating default filters")
  //    CPluginFilter::CreateFilters();
#endif

  // Force creation of default dictionary
  CPluginDictionary* dictionary = CPluginDictionary::GetInstance(true);
  dictionary->Create(true);

  // Force creation of default config file
#ifdef SUPPORT_CONFIG
  DEBUG_GENERAL("*** Generating config file")
    CPluginConfig* config = CPluginConfig::GetInstance();
  config->Create(true);
#endif

  HKEY hKey = NULL;
  DWORD dwDisposition = 0;

  DWORD dwResult = NULL;

  // Post async plugin error
  CPluginError pluginError;
  while (CPluginClientBase::PopFirstPluginError(pluginError))
  {
    CPluginClientBase::LogPluginError(pluginError.GetErrorCode(), pluginError.GetErrorId(), pluginError.GetErrorSubid(), pluginError.GetErrorDescription(), true, pluginError.GetProcessId(), pluginError.GetThreadId());
  }
}

// Called from installer
EXTERN_C void STDAPICALLTYPE OnInstall(MSIHANDLE hInstall, MSIHANDLE tmp)
{
  InitPlugin(true);
}

// Called from updater
EXTERN_C void STDAPICALLTYPE OnUpdate(void)
{
  InitPlugin(false);
}