#include <windows.h>
#include <stdio.h>
#include <Winsvc.h>
#define SLEEP_TIME 5000  // the time period in milliseconds between two consecutive queries for available memory
#define LOGFILE "D:\\Test Service Log.txt" 


SERVICE_STATUS ServiceStatus; 
SERVICE_STATUS_HANDLE hStatus; 
void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();


 /*
 * Function:  WriteToLog 
 * ------------------------
 * write the amount of available physical memory in log file "C:\\Test Service Log.txt" 
 *
 *  param: available physical memory
 *
 *  returns: write in log file         
 */
int WriteToLog(char* str)
{
	FILE* log;
	log = fopen(LOGFILE, "a+");
	if (log == NULL)
		return -1;
	fprintf(log, "%s\n", str);
	fclose(log);
	return 0;
}




/*
 * Function:  ServiceMain 
 * -----------------------------
 * Entry point of a service , It runs in a separate thread
 *
 *  param:  service name &  pointer to the ControlHandlerfunction
 */

void ServiceMain(int argc, char** argv) 
{ 
    int error; 
 
    ServiceStatus.dwServiceType        = SERVICE_WIN32_OWN_PROCESS; 
    ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode      = 0; 
    ServiceStatus.dwServiceSpecificExitCode = 0; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0; 
 
    hStatus = RegisterServiceCtrlHandler(
		"MemoryStatus2", 
		(LPHANDLER_FUNCTION)ControlHandler); 
    if (hStatus == (SERVICE_STATUS_HANDLE)0) 
    { 
        // Registering Control Handler failed
        return; 
    }  
    error = InitService();     // Initialize Service 

    if (error) 
    {

        ServiceStatus.dwCurrentState       = SERVICE_STOPPED;	 // Initialization failed
        ServiceStatus.dwWin32ExitCode      = -1; 
        SetServiceStatus(hStatus, &ServiceStatus); 
        return; 
    } 
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;     // report the running status to SCM. 
    SetServiceStatus (hStatus, &ServiceStatus);
 
    MEMORYSTATUS memory;
    // The worker loop of a service
    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		char buffer[16];
		GlobalMemoryStatus(&memory);
		sprintf(buffer, "%d", memory.dwAvailPhys);
		int result = WriteToLog(buffer);
		if (result)
		{
			ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
			ServiceStatus.dwWin32ExitCode      = -1; 
			SetServiceStatus(hStatus, &ServiceStatus);
			return;
		}

		Sleep(SLEEP_TIME);
	}
    return; 
}
 




/*
 * Function:  InitService 
 * --------------------
 * proceed to the initialization log file
 *   
 *  returns: adds the Monitoring started string to the log file
 */
int InitService() 
{ 
    int result;
    result = WriteToLog("Monitoring started.");
    return(result); 
} 


/*
 * Function:  StartSvc 
 * --------------------
 *  Start the service after installation
 *    
 */
void StartSvc()
{
	SC_HANDLE serviceControlManager = OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
	SC_HANDLE serviceHandle = OpenService(serviceControlManager,"Test Service" , SERVICE_ALL_ACCESS );

	StartService(serviceHandle,0,NULL);

	CloseServiceHandle(serviceControlManager); 
        CloseServiceHandle(serviceHandle);
        return; 
}




/*
 * Function:  ControlHandler 
 * --------------------
 * It checks what request was sent by the SCM and acts accordingly
 *
 *  param: SCM control request
 *
 */
void ControlHandler(DWORD request) 
{ 
    switch(request) 
    { 
        case SERVICE_CONTROL_STOP: 
             WriteToLog("Monitoring stopped.");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
 
        case SERVICE_CONTROL_SHUTDOWN: 
            WriteToLog("Monitoring stopped.");

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
        
        default:
			StartSvc();
            break;
    } 
 
    SetServiceStatus (hStatus,  &ServiceStatus);    // Report current status

    return; 
} 


/*
 * Function:  InstallService 
 * --------------------
 * Installing and configuring a Service
 */
void InstallService()
{
	 SC_HANDLE serviceControlManager = OpenSCManager( 0, 0, SC_MANAGER_CREATE_SERVICE ); /* connect to SCM and tell we intend to create a service */

	if ( serviceControlManager )
	{
		TCHAR path[ _MAX_PATH + 1 ];
		if ( GetModuleFileName( 0, path, sizeof(path)/sizeof(path[0]) ) > 0 )  /* get the name of the .exe that we are running as */

		{
			SC_HANDLE service = CreateService( serviceControlManager,    			/* register this executable as a service */
							"Test Service", "Test Service",
							SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
							SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,
							0, 0, 0, 0, 0 );
			if ( service )
				CloseServiceHandle( service );
		}

		CloseServiceHandle( serviceControlManager );
	}
}




void main() 
{ 
    SERVICE_TABLE_ENTRY ServiceTable[2];
    ServiceTable[0].lpServiceName = "Test Service";
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;
    StartServiceCtrlDispatcher(ServiceTable);      // Start the control dispatcher thread for our service

	InstallService();
	StartSvc();

}