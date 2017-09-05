a. Your Full Name  as given in the class list
	
	Name: Yunxi Liu


b. Your Student ID
	
	ID: xxxxxxxxxx


c. What you have done in the assignment
	
	I have implemented a client to send files, a edge server for to handle the message transmission, and two back-end servers for 'AND' and 'OR' operations.


d. What your code files are and what each one of them does. (Please do not repeat the project description, just name your code files and briefly mention what they do).
	
	There are four files contained in my projects.
	1. client.cpp This program acts as the client, which is able to set up a TCP connection with the edge server and send the file through the connection.
	2. edge.cpp This program acts as the server, which is able to set up a TCP connection with the client to receive and transfer files, and open a UDP socket to send and receive results from the two back-end servers.
	3. server_or.cpp This program acts as the back-end-or server, which is able to open a udp socket to receive and transfer file. Also, it is the server to do 'OR' calculations.
	4. server_and.cpp This program acts as the back-end-and server, which is able to open a udp socket to receive and transfer file. Also, it is the server to do 'AND' calculations.


e. What the TA should do to run your programs. (Any specific order of events should be mentioned.)

	1. Download my zip and decompress them into a directory
	2. Open the command terminal, direct to the directory, type 'make all' to compile the four cpp programs.
	3. Open another three command terminals, direct to the directory, type ‘make server_or' in the one terminal, ‘make server_and' in another terminal, and ‘make edge' in another terminal. Then type './client <filename>.txt' in another terminal. Then the programs runs.


f. The format of all the messages exchanged.

	The format is char arrays, which is the parameter in send(), recv(), sendto() and recvfrom().


g. Any idiosyncrasy of your project. It should say under what conditions the
project fails, if any.

	The file format (<filename.txt>) must be strictly formatted. Namely, every line ends with a '\n', the last line might or might not end with '\n', and there should be no empty lines. The file size should also be within limitation. Namely, every line should contains no more than 100 characters and there should be no more than 100 lines.
	When you compile using 'make' command, you may get warnings. DO NOT worry!!! Just go on and execute them as usual.


h. Reused Code: Did you use code from anywhere for your project? If not, say so. If so, say what functions and where they're from. (Also identify this with a comment in the source code.)

	I referred a little piece of code in the socket creation from beej's tutorial. I already commented them in my source code.
