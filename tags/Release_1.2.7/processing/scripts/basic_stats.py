# This script calculates how many error reports are in each subdirectory
# and how many error reports are in total.
# Edit in_dir and out_file parameters as you need.

import os

in_dir = "D:/Projects/CrashRpt/valid_reports"
out_file = "stats.txt"

f = open(out_file, "w")

def get_txt_file_count(dirname):
   count = 0
   for root, dirs, files in os.walk(dirname, True):
     for file in files:
        if file[-4:] != ".txt":
            continue;
        count += 1
     break;
   return count

map = dict()
for root, dirs, files in os.walk(in_dir):
   for dir in dirs:
      dir_name = os.path.join(root, dir)
      report_count_in_dir = get_txt_file_count(dir_name)
      map[report_count_in_dir] = dir

ordered_list = list(map.keys())
ordered_list.sort()
ordered_list.reverse()

total_count = 0
for count in ordered_list:
   total_count += count

f.write("%d reports in total \n"%(total_count))

for key in ordered_list:
   f.write("%d reports in '%s'\n"%(key, map[key]))

f.close()