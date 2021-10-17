#! /usr/bin/env python3

import shutil
import sys
import http
import http.server
import urllib.request
import posixpath
import os

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        p=self.path
        print("path=%s"%p)
        if p.startswith("/api/"):
            apiurl=self.server.proxyUrl
            url=apiurl+p.replace("/api","")
            print("proxy to %s"%url)
            with urllib.request.urlopen(url) as response:
                self.send_response(http.HTTPStatus.OK)
                self.send_header("Content-type", response.getheader("Content-type"))
                
                self.end_headers()
                shutil.copyfileobj(response,self.wfile)
                return None
            self.send_error(http.HTTPStatus.NOT_FOUND, "api not found")
            return None
        super().do_GET()        
    def translate_path(self, path):
        """Translate a /-separated PATH to the local filename syntax.

        Components that mean special things to the local file system
        (e.g. drive or directory names) are ignored.  (XXX They should
        probably be diagnosed.)

        """
        # abandon query parameters
        path = path.split('?',1)[0]
        path = path.split('#',1)[0]
        # Don't forget explicit trailing slash when normalizing. Issue17324
        trailing_slash = path.rstrip().endswith('/')
        try:
            path = urllib.parse.unquote(path, errors='surrogatepass')
        except UnicodeDecodeError:
            path = urllib.parse.unquote(path)
        path = posixpath.normpath(path)
        words = path.split('/')
        words = filter(None, words)
        path = self.server.baseDir
        for word in words:
            if os.path.dirname(word) or word in (os.curdir, os.pardir):
                # Ignore components that are not a simple file/directory name
                continue
            path = os.path.join(path, word)
        if trailing_slash:
            path += '/'
        return path

def run(baseDir,port,apiUrl,server_class=http.server.HTTPServer, handler_class=RequestHandler):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    httpd.proxyUrl=apiUrl
    httpd.baseDir=baseDir
    httpd.serve_forever()

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("usage: %s basedir port apiurl"%sys.argv[0])
        sys.exit(1)
    run(sys.argv[1],int(sys.argv[2]),sys.argv[3])    