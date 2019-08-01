// printspoolstatus.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	cout << "Initializing..."<<endl;
	
	//Declare variables
	typedef UINT(CALLBACK* LPFNDLLFUNC1)(PRINT_EXECUTION_DATA *);
	PRINT_EXECUTION_DATA * execData;
	int context_num = 999;
	bool result;
	DWORD buffer_size = 0;
	HDC default_printer_context;
	DWORD buffer_needed = 0;
	DWORD number_returned = 0;
	string current_printer;
	DWORD error_code;
	HANDLE printer_handle;
	HANDLE p_heap;
	HMODULE driver_handle;
	LPDRIVER_INFO_2 driver_info = NULL;
	LPDEVMODE dev_mode_in = NULL;
	LPDEVMODE dev_mode_out = NULL;
	LPJOB_INFO_2 job_info = NULL;
	DWORD jobs_returned = 0;
	LPDEVMODE dev_mode_lock = NULL;
	LPPRINTER_INFO_2 printer_enum_ptr = NULL;
	LPJOB_INFO_2 job_info_ptr = NULL;
	LPFNDLLFUNC1 GetPrintStatus =NULL;
	int arg_count;
	bool all = false;

	//Get command line arguments
	for (arg_count = 0; arg_count < argc; arg_count++) {
		if (string(argv[arg_count]) == "all") {
			all = true;
		}
	}

	// Create process heap
	p_heap = HeapCreate(0, 0, 0);
	error_code = GetLastError();

	if (p_heap == NULL) {
		cout << "Heap creation failed." << endl;
		cout << "Error code: " << error_code << endl;
		return 1;
	}

	// Enumerate all printers

	result = EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &buffer_needed, &number_returned);

	LPPRINTER_INFO_2 printer_enumeration = (LPPRINTER_INFO_2)malloc((long)buffer_needed);
	error_code = GetLastError();

	if (printer_enumeration == NULL) {
		cout << "Printer buffer initialization failed. Exiting." << endl;
		cout << "Error code: " << error_code << endl;
		return 1;
	}
	printer_enum_ptr = printer_enumeration;

	result = EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)printer_enumeration, buffer_needed, &buffer_needed, &number_returned);
	error_code = GetLastError();

	if (result == false) {
		cout << "Printer enumeration failed. Exiting." << endl;
		cout << "Error code: " << error_code << endl;
		return 1;
	}


	// Print Header
	cout << "Device Name, Job Name, Job Status, Spooler Context, WOW64PID" << endl;
	for (int i = 0; i < (int)number_returned; i++) {
		driver_handle = NULL;
		driver_info = NULL;
		result = false;
		printer_handle = NULL;
	
		dev_mode_in = NULL;
		dev_mode_out = NULL;
		jobs_returned = 0;
		job_info = NULL;
		execData = NULL;
		job_info_ptr = NULL;


		// Find printer handle
		result = OpenPrinter(printer_enum_ptr->pPrinterName, &printer_handle, NULL);
		error_code = GetLastError();

		if (result == false) {
			cout << "Open printer device " << printer_enum_ptr->pPrinterName <<"failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		// Get driver path
		result = GetPrinterDriver(printer_handle, NULL, 2, NULL, 0, &buffer_needed);

		driver_info = (LPDRIVER_INFO_2)HeapAlloc(p_heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, (long)buffer_needed);
		error_code = GetLastError();
		if (driver_info == NULL) {
			cout << "Allocating buffer for driver info failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		result = GetPrinterDriver(printer_handle, NULL, 2, (LPBYTE)driver_info, buffer_needed, &buffer_needed);
		error_code = GetLastError();
		if (result == false) {
			cout << "Getting driver info failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		// Load library
		driver_handle = LoadLibrary(driver_info->pDriverPath);
		error_code = GetLastError();
		if (driver_handle == NULL) {
			cout << "Getting driver handle failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		// Get printer devmode
		buffer_needed = DocumentProperties(NULL, printer_handle, printer_enum_ptr->pPrinterName, NULL, NULL, 0);


		dev_mode_out = (LPDEVMODE)HeapAlloc(p_heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, (long)buffer_needed);
		error_code = GetLastError();
		if (dev_mode_out == NULL) {
			cout << "Allocating dev mode out buffer failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		dev_mode_in = (LPDEVMODE)HeapAlloc(p_heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, (long)buffer_needed);
		error_code = GetLastError();
		if (dev_mode_in == NULL) {
			cout << "Allocating dev mode in buffer failed. Exiting." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		buffer_needed = DocumentProperties(NULL, printer_handle, printer_enum_ptr->pPrinterName, dev_mode_out, dev_mode_in, DM_OUT_BUFFER);
		error_code = GetLastError();
		if (buffer_needed != IDOK) {
			cout << "Getting dev mode failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		
		// Get all jobs for current printer
		result = EnumJobs(printer_handle, 0, printer_enum_ptr->cJobs, 2, NULL, 0, &buffer_needed, &jobs_returned);

		job_info = (LPJOB_INFO_2)malloc((long)buffer_needed);
		error_code = GetLastError();
		if (job_info == NULL) {
			cout << "Allocating buffer for print jobs failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}

		result = EnumJobs(printer_handle, 0, printer_enum_ptr->cJobs, 2, (LPBYTE)job_info, buffer_needed, &buffer_needed, &jobs_returned);
		error_code = GetLastError();
		if (result == false) {
			cout << "Allocating buffer for print jobs failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		job_info_ptr = job_info;

		if ((long)jobs_returned > 0) {

			for (int j = 0; j < (long)jobs_returned; j++) {
				execData = NULL;
				context_num = 999;

				// Get device context

				default_printer_context = CreateDC(L"WINSPOOL", printer_enum_ptr->pPrinterName, 0, job_info_ptr->pDevMode);
				error_code = GetLastError();
				if (default_printer_context == NULL) {
					cout << "Create context failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}

				//Get execution data
				GetPrintStatus = (LPFNDLLFUNC1)GetProcAddress(GetModuleHandle(TEXT("winspool.drv")), "GetPrintExecutionData");
				error_code = GetLastError();
				if (GetPrintStatus == NULL) {
					cout << "Getting address of GetPrintExecution data failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}

				execData = (PRINT_EXECUTION_DATA *)HeapAlloc(p_heap, 0, 262144);
				error_code = GetLastError();
				if (execData == NULL) {
					cout << "Allocating buffer for print data failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}
				result = GetPrintStatus(execData);
				error_code = GetLastError();

				if (result == false) {
					cout << "Getting execution data failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}
				wcout << printer_enum_ptr->pPrinterName << ", " << job_info_ptr->pDocument << ", ";
				if (job_info->pStatus == NULL) {
					if (job_info_ptr->Status & JOB_STATUS_BLOCKED_DEVQ) {
						cout << "JOB_STATUS_BLOCKED_DEVQ, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_DELETED) {
						cout << "JOB_STATUS_DELETED, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_DELETING) {
						cout << "JOB_STATUS_DELETING, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_ERROR) {
						cout << "JOB_STATUS_ERROR, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_OFFLINE) {
						cout << "JOB_STATUS_OFFLINE, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_PAPEROUT) {
						cout << "JOB_STATUS_PAPEROUT, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_PAUSED) {
						cout << "JOB_STATUS_PAUSED, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_PRINTED) {
						cout << "JOB_STATUS_PRINTED, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_PRINTING) {
						cout << "JOB_STATUS_PRINTING, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_RESTART) {
						cout << "JOB_STATUS_RESTART, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_SPOOLING) {
						cout << "JOB_STATUS_SPOOLING, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_USER_INTERVENTION) {
						cout << "JOB_STATUS_USER_INTERVENTION, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_COMPLETE) {
						cout << "JOB_STATUS_COMPLETE, ";
					}
					else if (job_info_ptr->Status & JOB_STATUS_RETAINED) {
						cout << "JOB_STATUS_RETAINED, ";
					}
					else {
						cout << "UNKNOWN, ";
					}

				}
				else {
					wcout << job_info_ptr->pStatus << ", ";
				}
				if (result == true) {

					context_num = execData->context;

					if (context_num == PRINT_EXECUTION_CONTEXT_APPLICATION) {
						wcout << "Application" << endl;
					}
					else if (context_num == PRINT_EXECUTION_CONTEXT_SPOOLER_SERVICE) {
						wcout << "Service" << endl;
					}
					else if (context_num == PRINT_EXECUTION_CONTEXT_SPOOLER_ISOLATION_HOST) {
						wcout << "Print isolation host" << endl;
					}
					else if (context_num == PRINT_EXECUTION_CONTEXT_FILTER_PIPELINE) {
						wcout << "Print filter pipeline" << endl;
					}
					else if (context_num == PRINT_EXECUTION_CONTEXT_WOW64) {
						wcout << "WOW64,";
						wcout << "Client app PID: " << execData->clientAppPID << endl;
					}
					else {
						wcout << "Unknown state returned:  " << context_num << endl;
					}
				}
				else {
					cout << "Error encountered." << endl;

				}
				job_info_ptr++;
				result = DeleteDC(default_printer_context);
				error_code = GetLastError();
				if (result == false) {
					cout << "Delete context failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}

				result = HeapFree(p_heap, 0, execData);
				error_code = GetLastError();
				if (result == false) {
					cout << "Freeing memory for execution data failed." << endl;
					cout << "Error code: " << error_code << endl;
					return 1;
				}
			}
		}
		else if (all == true) {
			context_num = 999;
			execData = NULL;

			// Get device context
			default_printer_context = CreateDC(L"WINSPOOL", printer_enum_ptr->pPrinterName, 0, printer_enum_ptr->pDevMode);
			error_code = GetLastError();
			if (default_printer_context == NULL) {
				cout << "Create context failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}

			//Get execution data
			// GetModuleHandle(TEXT("winspool.drv"))
			GetPrintStatus = (LPFNDLLFUNC1)GetProcAddress(GetModuleHandle(TEXT("winspool.drv")), "GetPrintExecutionData");
			error_code = GetLastError();
			if (GetPrintStatus == NULL) {
				cout << "Getting address for GetPrintExecutionData failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}

			execData = (PRINT_EXECUTION_DATA *)HeapAlloc(p_heap, 0, 262144);
			error_code = GetLastError();
			if (execData == NULL) {
				cout << "Allocate buffer for execution data failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}

			result = GetPrintStatus(execData);

			error_code = GetLastError();

			if (result == false) {
				cout << "Getting execution data failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}
			wcout << printer_enum_ptr->pPrinterName << ", NO_JOB, ";
			if (result == true) {

				context_num = execData->context;

				if (context_num == PRINT_EXECUTION_CONTEXT_APPLICATION) {
					wcout << "Application" << endl;
				}
				else if (context_num == PRINT_EXECUTION_CONTEXT_SPOOLER_SERVICE) {
					wcout << "Service" << endl;
				}
				else if (context_num == PRINT_EXECUTION_CONTEXT_SPOOLER_ISOLATION_HOST) {
					wcout << "Print isolation host" << endl;
				}
				else if (context_num == PRINT_EXECUTION_CONTEXT_FILTER_PIPELINE) {
					wcout << "Print filter pipeline" << endl;
				}
				else if (context_num == PRINT_EXECUTION_CONTEXT_WOW64) {
					wcout << "WOW64,";
					wcout << "Client app PID: " << execData->clientAppPID << endl;
				}
				else {
					wcout << "Unknown state returned:  " << context_num << endl;
				}
			}
			else {
				cout << "Error encountered." << endl;

			}
			result = DeleteDC(default_printer_context);
			error_code = GetLastError();
			if (result == false) {
				cout << "Delete context failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}
			result = HeapFree(p_heap, 0, execData);
			error_code = GetLastError();
			if (result == false) {
				cout << "Freeing memory for execution data failed." << endl;
				cout << "Error code: " << error_code << endl;
				return 1;
			}
		}

		result = FreeLibrary(driver_handle);
		error_code = GetLastError();
		if (result == false) {
			cout << "Freeing memory for driver library failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		result = ClosePrinter(printer_handle);
		error_code = GetLastError();
		if (result == false) {
			cout << "Close printer failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		HeapFree(p_heap, 0, dev_mode_in);
		error_code = GetLastError();
		if (result == false) {
			cout << "Freeing memory for dev mode in failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		HeapFree(p_heap, 0, dev_mode_out);
		error_code = GetLastError();
		if (result == false) {
			cout << "Freeing memory for dev mode out failed." << endl;
			cout << "Error code: " << error_code << endl;
			return 1;
		}
		if (job_info) {
			free(job_info);
		}

		printer_enum_ptr++;

	}

	HeapFree(p_heap, 0, driver_info);
	free(printer_enumeration);
	result = HeapDestroy(p_heap);
    return 0;
}

