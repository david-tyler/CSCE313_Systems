/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: David-Tyler Ighedosa
	UIN: 231008755
	Date: 9/24/2023
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <fstream>
#include <iostream>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 0;
	double t = -1.0;
	int e = 1;
	int m = MAX_MESSAGE; //buffer cap
	bool new_chan = false;
	bool filetransfer = false;
	//vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filetransfer = true;
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
			
		}
	}

	// give arguments for the server
	//server needs './server', '-m', '<val for -m arg>', 'NULL'
	//fork
	//In the child run excvp using the server arguments.
	if (fork() == 0){
		char* args [] = {(char *) "./server", (char *)"-m", (char *) to_string(m).c_str(), NULL};
		if (execvp (args[0], args) < 0){
			perror("exec failed");
			exit(0);
		}
	}



    FIFORequestChannel* cont_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* chan = cont_chan;
	//channels.push_back(&cont_chan);
	if (new_chan){
		//send newchannel request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    	cont_chan->cwrite(&nc, sizeof(MESSAGE_TYPE));
		//create a variable to hold the name
		//cread the response from the server
		//call the FIFORequestChannel contstructor with the name from the server
		//push the new channel into the vector
		char newchanName [100];
		cont_chan->cread(newchanName, sizeof(newchanName));
		chan = new FIFORequestChannel (newchanName, FIFORequestChannel::CLIENT_SIDE);
	}
	//FIFORequestChannel chan = *(channels.back());
	
	//Single data point only run when p, t, e, != -1
	// example data point request
	if (!filetransfer){
		//char buf[MAX_MESSAGE]; // 256
		//datamsg x(1, 0.0, 1); //change from hardcoding to user's values
		
		//memcpy(buf, &x, sizeof(datamsg));
		//chan.cwrite(buf, sizeof(datamsg)); // question
		
		//Else if p != -1, request 1000 data points
		//loop over 1st 1000 lines
		//send request for ecg 1
		//send request for ecg 2
		//write line to receive/x1.csv
		if (t >= 0){
			datamsg d(p, t, e);
			chan->cwrite (&d, sizeof(d));
			double ecg; //actual value
			chan->cread(&ecg, sizeof(double));
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << ecg << endl;
			MESSAGE_TYPE q = QUIT_MSG;
			chan->cwrite(&q, sizeof(MESSAGE_TYPE));

			if (chan != cont_chan){ //user requested a new channel so control must be destroyed as well
				cont_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
				delete cont_chan;
				
			}
			delete chan;
			wait(0);
			return 0;
		}
		else{ //1000 requests
			ofstream outfile;
			outfile.open("received/x1.csv");
			double ts = 0;
			datamsg d(p, ts, 1);
			datamsg d2(p, ts, 2);
			double ecg1; //actual value
			double ecg2; 
			
			
			for (int i = 0; i < 1000; i++)
			{
				double sec = d.seconds;
				chan->cwrite(&d, sizeof(d));
				chan->cread(&ecg1, sizeof(double));
				d.ecgno += 1;
				chan->cwrite(&d, sizeof(d));
				chan->cread(&ecg2, sizeof(double));
				outfile << sec << "," << ecg1 << "," << ecg2 << endl;
				
				d.seconds += 0.004;
				d.ecgno -= 1;
				
			}
			outfile.close();
			MESSAGE_TYPE q = QUIT_MSG;
			chan->cwrite(&q, sizeof(MESSAGE_TYPE));

			if (chan != cont_chan){ //user requested a new channel so control must be destroyed as well
				cont_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
				delete cont_chan;
			}
			delete chan;

			wait(0);
			return 0;
		}
	}
	else if(filetransfer){
		//requesting a file
		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());
		chan->cwrite(buf2, len);  // I want the file length;




		int64_t filesize = 0;
		chan->cread(&filesize, sizeof(int64_t));
		//loop over the segments in the file, filesize/buffer capacity(m).
		//create filemsg instance can reuse buf2 or not
		//^ do this by 
		filemsg* file_req = (filemsg*)buf2;		
		file_req->offset = 0; //set offset in the file
		file_req->length = MAX_MESSAGE; //set the length. Be careful of the last argument
		string outPath = string("received/") + filename;
		FILE* outfile = fopen (outPath.c_str(), "wb");
		int64_t remainder = filesize;

		char* recvbuff = new char[MAX_MESSAGE]; //create buffer of size buff capactiy(m)
		while (remainder > 0){
			file_req->length = (int) min(remainder, (int64_t) MAX_MESSAGE);
			chan->cwrite(buf2, len);
			chan->cread(recvbuff, MAX_MESSAGE);
			fwrite (recvbuff, 1, file_req->length, outfile);
			remainder -= file_req->length;
			file_req->offset += file_req->length;

		}
		fclose(outfile);
		delete[] buf2;
		delete[] recvbuff;

		//send the request (buf2) if reusing buf2
		//receive the response
		//cread into buf3. length file_req->len
		//write buf3 into file received/filename

		

	}
    




   
	
	//if necessary close and delete the new channel
	//if (new_chan)
	//{
		//do your close and deletes
	//}
	// closing the channel    

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite(&q, sizeof(MESSAGE_TYPE));

	if (chan != cont_chan){ //user requested a new channel so control must be destroyed as well
		cont_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
		delete cont_chan;
	}
	delete chan;

	wait(0);
}
