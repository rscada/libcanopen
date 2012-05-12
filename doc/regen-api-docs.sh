grep -h "//ST " ../src/*.[ch] | sed 's/\/\/ST //g' > source/api-types.rst
grep -h "//SF " ../src/*.[ch] | sed 's/\/\/SF //g' > source/api-functions.rst
grep -h "//S " ../bin/*.[ch] | sed 's/\/\/S //g' > source/programs-list.rst

