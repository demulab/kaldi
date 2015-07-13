#!/usr/bin/env python
#from signal import signal, SIGPIPE, SIG_DFL
#signal(SIGPIPE, SIG_DFL) 
import subprocess, re, os, sys
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import threading
import cgi
from datetime import datetime

dlog = './input-log/'

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

def shback(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    return p

def shcin(stdin_data, server):
    server.stdin.write(stdin_data)

    out = server.stdout.readline()
    return out

class Handler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        message =  threading.currentThread().getName()
        self.wfile.write(message)
        self.wfile.write('\n')
        return

    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                    })

        t = datetime.now()
        filename = 'input-' + str(t.hour) + str(t.minute) + str(t.second) + '.wav'
        filepath = dlog + filename
        with open(filepath, 'wb') as f:
            f.write(form.value)

        cmd = '~/myprog/src/kaldi/scripts/convert_sample_rate.sh %s %s8k_%s' % (filepath, dlog, filename)
        shcom(cmd)
        
        filename = dlog + "8k_" + filename + '\n'
        res = shcin(filename, server_process)
	
        answer = ('%s\n' % res)
        self.wfile.write(answer)

        return

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""

if __name__ == '__main__':
    cmd = 'mkdir -p ' + dlog
    shcmd(cmd)
    host = "localhost" # sys.argv[1]
    port = 8081        # int(sys.argv[2])
    server = ThreadedHTTPServer((host, port), Handler)

    # Start the kaldi server
    cmd = '~/myprog/src/kaldi/scripts/server.sh'
    global server_process
    server_process = shback(cmd)

    print 'Starting server on %s:%s, use <Ctrl-C> to stop' % (host, port)
    server.serve_forever()
