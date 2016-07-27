# BitTorrent_FileSharing

Program Introduction
	Our project is aimed to accomplish the file sharing using Bit Torrent and Chord technology. There are two major features: upload/download and tracker management.

Upload and Download:
Uploading:
	In Bit-torrent, the person who creates the initial copy of the file is named Seed. It will create .torrent file and uploads to a web server: contains metadata about tracker and about the pieces of the file. In our program we make our tracker file to include the following information: the filename, the number of total peers (including the seed) and a user array contains each peer’s IP and Port (Peers are the user who have own the complete copy of the file including the seed).

Downloading:
	In Bit-torrent, Peer downloads .torrent file and contacts server, then server returns random set of peers, including seed and leechers. Peer downloads from seed, eventually from leechers. In our Program, user will run the torrent client to contact the chord program, which serves as the web server to manage the tracker information and will get the response about “How many peers have the file and what are their contact information”. After getting the contact info, the client can spawn the same pieces numbers of threads to download the corresponding part of the file from different peers. After all pieces are successfully downloaded, the program will concatenate the pieces and the user will have the original file. As long as the client program is still alive, it will automatically contact the chord, and ask it to update the tracker information, adding a new pair of IP and Port to the specific file.
To be mentioned, a user can re-upload and re-download the same file.

Tracker management:
	In the chord program, each node will have a tracker array to store different trackers info, which related to the specific filenames. We have hashed the filename to decide in which node the tracker info will be stored. So when the user uploads a file, the torrent client actually sends a message to a node in chord, and let it add relevant information to the trackers. And after the user downloads a file, the client will also send a similar message to a node to update the tracker. 
We here have considered about tracker update when node quit and user quit. In first situation, the node can transfer all its trackers to its successor after exiting, and when the node comes back, or when a new node comes in, it can get relevant trackers from its successor. The other situation is that a user exits from the torrent program. Once running torrent program, the program itself will record the user’s upload or download operation, until user exits (just like a log). So after user exits, the torrent program can then send message to chord to delete relevant pieces of trackers. We can’t deal with abrupt exit, for both chord node and torrent client. Also, when a user restart torrent client, he should re-upload the files he has to update the trackers.

How to run this program
Below are the instructions about how to compile and run our program:
Step 1:
	In both the “torrent” folder and the “chord” folder, type “make”, it will generate two executable files named “torrent” and “chord”, separately.
Step 2:
	For the Chord: You can type the following commands "./chord [your port]”(e.g. ./chord 8001) to set up the chord ring or type "./chord [your port] [server IP] [server port]” (e.g. ./chord 8001 128.2.205.42 8010) to join one existed chord ring. After repeatedly doing so, a chord ring with many nodes will be successfully set up. After setting up the chord ring, you can look up the node's specific information by typing "my", "list", “table" or “tracker” in the terminal. Also you can type “quit” to quit peacefully.
	For the torrent: If you just want to test our program in one computer, please ignore this paragraph. If you want to test the program on different machines, please read the following words carefully. Before running the torrent program, you should decide whether you are using a virtual Linux machine (like Ubuntu) or not. Because for virtual machine you might need to change some setting in order to link the physical machine’s IP with virtual machine’s IP through a specific port number. For example, in parallel desktop, all you need to change is add port forwarding rules and set the Port on Mac and Destination to be the port number you want to use. (See picture below)
 

Then, type “./torrent [your port]” to run the program, you will be asked whether you are running a virtual machine and you should type in your IP and Port information. After that, you can decide if you want to upload or download.  

Note
	In order to guarantee the program is able to accomplish downloading and uploading files between computers, you may have to change the “chord_ip” and “chord_port” to ensure that the torrent program can upload tracker information to the chord ring correctly. We have set the default value of “chord_ip” to be “127.0.0.1” and default value of “chord_port” to be 8999(It represents the node you contact with, i.e. master node). Besides, the master node servers as the most important node in the ring, every query will go through this node, so this node can’t be offline when user upload or download files. 
Final attention, please use “quit” command to exit chord node and torrent client, to make sure trackers can be updated correctly.
If you have question, feel free to contact us and we are very willing to show you the program because file transfer may need several different computers.
