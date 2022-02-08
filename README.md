# Usage: 
test-1.cpp - Build and run

telnet 127.0.0.1 8082

Command:
1. get
2. sort_id
3. sort_s1
4. sort_s2

Optional enviroiment:

1. max_row - Maximum row to fetch from server
2. start_row - Set the ID that will be the beginning of the part of the table
3. Delimeter to enviroiment - ";"

Example:

1. get - get N row  by sort from previous command
2. sort_s1 - get N row sort by column 
3. max_row=100;start_row=1000000;get - get 100 row from row with id = 1000000
