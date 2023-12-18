// your PA3 client code here

// change all instances of FIFOReqChan to TCPReqChan
// change to the appropriate constructor 

//getopt, add a and r and creaete variables for these values


#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>
//#include <utility>  

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

// ecgno to use for datamsgs
#define EGCNO 1

using namespace std;


void patient_thread_function (/* add necessary arguments */ int p_no, BoundedBuffer *request_buffer, int n, int buffer_capacity) {
    // functionality of the patient threads

    // take a patient p_no
    // for n requests, produce a datamsg(p_no, time, ECGNO)
    //      - time dependent on current requests
    //      - at 0 -> time - 0.000; at 1 -> time = 0.004; at 2 -> time = 0.008;
    
    double time = 0;
    for(int i = 0; i < n; i++)
    {
        datamsg d(p_no, time, EGCNO);
        char *request = new char[buffer_capacity];
        memcpy(request, &d, sizeof(datamsg));
        request_buffer->push(request, buffer_capacity);
        delete [] request;
        time += 0.004;
    }
    //cout << " Pushing Datamsg" <<  endl;
}

void file_thread_function (/* add necessary arguments*/ BoundedBuffer* request_buffer, string filename, TCPRequestChannel* chan, int buffer_capacity) {
    // functionality of the file thread
    //PA1 code
    // - all the lines except 187-189 will be used.those 3 lines will be replaced with pushing into the buffer
    //first get the filesize -> channel
    //create received, file

    // file size - do that with the filemsg 00 appended with the filename
    // open output file; allocate the memory of the file with a fseek; close the file
    // while offset < file size, produce a filemsg(offset, m (<- length of the message)) +filename and push to request_buffer
    //      - incrementing offset; and be careful with the final message 

    filemsg fm(0, 0);
    int len = sizeof(filemsg) + (filename.size() + 1);
    char* buf2 = new char[buffer_capacity];
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
    file_req->length = buffer_capacity; //set the length. Be careful of the last argument

    string outPath = string("received/") + filename;
    FILE* outfile = fopen (outPath.c_str(), "wb");
    int64_t remainder = filesize % buffer_capacity;

    fseek(outfile, filesize, SEEK_SET);
    
    for(int i = 0; i < filesize - remainder; i+=buffer_capacity)
    {
        file_req->offset = i;
        
        // REPLACE BELOW 3 LINES WITH PUSHING INTO THE BUFFER
        //chan->cwrite(buf2, len);
        //chan->cread(recvbuff, MAX_MESSAGE);
        //fwrite (recvbuff, 1, file_req->length, outfile);

        //cout << " Pushing Filemsg " << file_req->mtype <<  endl;
        request_buffer->push(buf2, buffer_capacity);

    }
    if (remainder != 0)
    {
        //cout << " Pushing Filemsg2 "  << buf2 <<  endl;
        file_req->offset = filesize - remainder;
        file_req->length  = remainder;
        request_buffer->push(buf2, buffer_capacity);
    }        
    
    fclose(outfile);
    delete[] buf2;

}

void worker_thread_function ( BoundedBuffer* request_buffer, BoundedBuffer* response_buffer,  TCPRequestChannel* chan, string filename, int buffer_capacity ) {
    // functionality of the worker threads

    // produce requests until told to stop -> forever loop
    // pop message from request_buffer
     // view line 120 in server (process_request function) for how to decide current message
    while(true)
    {
        char* msg = new char[buffer_capacity];
        request_buffer->pop(msg, buffer_capacity);
        MESSAGE_TYPE m = *((MESSAGE_TYPE*) msg);
        
        // - send the message across a FIFO channel
        // - collect response
        // if DATA:
        //      - create pair of p_no from message and response from server
        //      - push that pair to the response_buffer
        if (m == DATA_MSG) 
        {
            
            datamsg dm = *(datamsg*) msg;
            double response;
            chan->cwrite(msg, sizeof(datamsg));
            
            chan->cread(&response, sizeof(double));
            
            
            std::pair <int , double>* pair1 = new pair<int, double>(dm.person, response);
            response_buffer->push((char*)pair1, sizeof(pair<int, double>));
            delete pair1;
            pair1 = nullptr;

        }
        else if (m == FILE_MSG) 
        {
            // if FILE:
            
            //      - collect the filename from the message popped form the buffer
            //      - open the file in update mode
            //      - fseek(SEEK_SET) to offset of the filemsg
            //      - write the buffer from the server

            // size of filemsg + filename.size() + 1. What we put into the buffer
            filemsg f = *(filemsg*) msg;

            int len = sizeof(filemsg) + (filename.size() + 1);
            
            //char* response = new char[buffer_capacity];
            chan->cwrite(msg, len);
            chan->cread(msg, buffer_capacity);
            FILE* file_obj = fopen(("received/" + filename).c_str(), "rb+");

            fseek(file_obj, f.offset, SEEK_SET);
            fwrite(msg, sizeof(char), f.length, file_obj);
            fclose(file_obj);
            //delete[] response;
        }
        else if (m == QUIT_MSG) 
        {
            chan->cwrite(msg, buffer_capacity);
            delete chan;
            delete[] msg;
            break;
            
        }
        delete[] msg;
    }
    
    
    
    
}

void histogram_thread_function (BoundedBuffer* response_buffer, HistogramCollection* hc) {
    // functionality of the histogram threads

    // forever looop
    // pop response from the response_buffer
    // call HC::update(resp->p_no, resp->double(response from the server))
    pair<int, double>* msg = new pair<int, double>;
    int msgsize = response_buffer->pop((char*)msg, sizeof(pair<int, double>));
    while(true)
    { 
        if(msg->first == -1 && msg->second == -1)
        {
            break;
        }
        hc->update(msg->first, msg->second);
        msgsize = response_buffer->pop((char*)msg, sizeof(pair<int, double>));
        
    }
    // << "Histogram done" << endl;
    delete msg;

        
}

// FIFORequestChannel* create_new_channel (FIFORequestChannel* mainchan)
// {
//     MESSAGE_TYPE nc = NEWCHANNEL_MSG;
//     mainchan->cwrite(&nc, sizeof(MESSAGE_TYPE));
//     char newchanName [100];
//     mainchan->cread(newchanName, sizeof(newchanName));
//     FIFORequestChannel *new_channel = new FIFORequestChannel(newchanName, FIFORequestChannel::CLIENT_SIDE);
//     return new_channel;
// }
int main (int argc, char* argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
    string port = "8080"; // default prot
    string host = "127.0.0.1"; // default address
	string f = "";	// name of file to be transferred
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
            case 'a':
                host = optarg;
                break;
            case 'r':
                port = optarg;
                break;
		}
	}
    
	// fork and exec the server
    // int pid = fork();
    // if (pid == 0) {
    //     execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    // }
    
	// initialize overhead (including the control channel)
	TCPRequestChannel *chan = new TCPRequestChannel(host, port); 
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    // array of producer threads (if data, p elements; if file, 1 element)
    // array of FIFOs (w elements)
    // array of worker threads (w elements)
    // array of histogram threads (if data, h elemnts; if file, 0 elements)
    vector <thread> worker_threads;
    vector <thread> producer_threads;
    vector <TCPRequestChannel*> TCPReqChansArr;
    vector <thread> histogram_threads;

    

    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
    
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* create all threads here */
    // if data:
    //      - create p patient_threads (store in producer array)
    //      - create h histogram_threads (store in histogram array)
    // if file:
    //      - create 1 file thread (store in producer array)
    
    // - create w worker_threads (store in worker array)
    //      -> create channel (store in FIFO array)

	/* join all threads here */
    // iterate over all thread arrays, calling join, order of joining matters
    //      -- producers before consumers
    //      -- patient threads -> worker threads -> histogram threads

    /* ------ DATA MSG -------*/
    if (f.empty()) {
        /* ----- Start all threads here ----- */
         // making histograms and adding to collection
         if (m > MAX_MESSAGE)
        {
           m = MAX_MESSAGE;
        }
        // create `p` patient threads
        for (int i = 0; i < p; i++) {
            producer_threads.push_back(move(thread(patient_thread_function, i+1, &request_buffer, n, m)));
        }    
        
        // create `w` new channels
        for (int i = 0 ; i < w ; i++) 
        {
            TCPRequestChannel *new_chan = new TCPRequestChannel(host, port); 
            TCPReqChansArr.push_back(new_chan);
        }

        // create `w` worker threads
        //worker_thread_function(&request_buffer, &response_buffer, FIFOs[0], f);
        for(int i = 0; i < w; i++) {
            worker_threads.push_back(move(thread(worker_thread_function, &request_buffer, &response_buffer, TCPReqChansArr[i], f, m)));
        }
        
        // create 'h' histogram threads
        for (int i = 0; i < h; i++)
        {
            histogram_threads.push_back(move(thread(histogram_thread_function, &response_buffer, &hc)));
        }
        

        /* ----- Join all threads here ----- */   

        // join the patient threads
        for (int i = 0; i < p; i++) {
            producer_threads[i].join();
        }
        
        // send QUIT_MSG's for each worker thread
        //MESSAGE_TYPE q_msg = QUIT_MSG;
        //cout << " Pushing Datamsg main" <<  endl;
        for (int i = 0; i < w; i++) {
            datamsg q (0, 0, 0);
            q.mtype = QUIT_MSG;
            char* buf = new char[m];
            memcpy(buf, &q, sizeof(QUIT_MSG));
            request_buffer.push(buf, m);
            delete[] buf;
        } 

        // join the worker threads
        for (int i = 0; i < w; i++) {
            worker_threads[i].join();
        }

        // send QUIT_MSG's for each histogram thread

        for (int i = 0; i < h; i++) {
            pair<int, double> q(-1, -1);

            response_buffer.push((char*) &q, sizeof(pair<int, double>));
        }
        //join histogram threads
        for (int i = 0; i < h; i++) {
            histogram_threads[i].join();
        }


    }
    /* ======= FILE_MSG Section ======= */
    else if(!f.empty())
    {
        if (m > MAX_MESSAGE)
        {
           m = MAX_MESSAGE;
        }
        // create `w` new channels
        for (int i = 0 ; i < w ; i++) 
        {
            TCPRequestChannel *new_chan = new TCPRequestChannel(host, port); 
            TCPReqChansArr.push_back(new_chan);
        }
        // create patient thread
        producer_threads.push_back(move(thread(file_thread_function, &request_buffer, f, chan, m)));
        
        
         

        // create `w` worker threads
        for(int i = 0; i < w; i++) {
            worker_threads.push_back(move(thread(worker_thread_function, &request_buffer, &response_buffer, TCPReqChansArr[i], f, m)));
        }

        // join the patient thread
        
        producer_threads[0].join();

        // send QUIT_MSG's for each worker thread
        for (int i = 0; i < w; i++) {
            
            filemsg q (0, 0);
            q.mtype = QUIT_MSG;
            char* buf = new char[m];
            memcpy(buf, &q, sizeof(filemsg));
            request_buffer.push(buf, m);
            delete[] buf;
        } 

        // join the worker threads
        for (int i = 0; i < w; i++) {
            worker_threads[i].join();
        }
    }




	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    

	// quit and close control channel
    
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;


	// wait for server to exit
	// wait(nullptr);
}
