# This script 
# Edit parameters below as you need before running the script.

import os
import fnmatch

input_dir = "valid_reports"
input_file_pattern = "*.zip"
acceptable_appname = ""
acceptable_appversion = ""
sym_search_dirs = ""
save_results_to_dir = "output"
save_invalid_reports_to_dir = "invalid_reports"
crprober_path = "D:\Projects\CrashRpt\branch_1.2\bin\crprober.exe"
log_file = "postprocess_log.txt"

def process_report(num, zip_path, hlog):
	file_name = os.path.basename(zip_path)
	hlog.write("%d. Processing report %s\n"%(num, file_name))
	if(acceptable_appname!=""):
		os.execv(crprober_path, [crprober_path, "/f", zip_path, "/o", "temp.txt", "/get", "XmlDescMisc", "AppName", "0"])
		if(os.EX_OK)


f = open(log_file, "w")

if(False==os.path.exists(save_results_to_dir)):
	os.mkdir(save_results_to_dir)


num = 0;
for root, dirs, files in os.walk(input_dir):
	for file in files:
		zip_name = os.path.join(root, file)
		if(fnmatch.fnmatch(zip_name, input_file_pattern)):
			num = num + 1
			process_report(num, zip_name, f)

f.close()