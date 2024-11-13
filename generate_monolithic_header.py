import os
import sys
dir_path = os.path.dirname(os.path.realpath(__file__))
filenames = [
	dir_path + "/vexcore/utils/CoreTemplates.h",
	dir_path + "/vexcore/containers/Tuple.h",
	dir_path + "/vexcore/containers/Union.h"
] 

out_file_name = 'containers_monolith.compexp.tmp';
out_file_processed_name = 'containers_monolith.compexp';
with open(out_file_name, 'w') as out_file:
    for fname in filenames:
        with open(fname) as infile:
            out_file.write(infile.read())

input_file = open(out_file_name, "r")
with open(out_file_processed_name, 'w') as out_file:
	for i, line in enumerate(input_file):
		if not line.startswith('#include'):
			if not line.startswith('#pragma'):
				out_file.write(line)

input_file.close()

