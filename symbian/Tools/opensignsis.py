'''opensignsis.py

OpenSignSIS is a Symbian OS Open Signed (Online) client

It allows you to automate the process of signing an
application using Open Signed Online service available
at: https://www.symbiansigned.com

Besides standard Python libraries it requires
wxPython for displaying a security code (captcha)
dialog.

opensignsis is copyright 2008, Arkadiusz Wahlig
<arkadiusz.wahlig@gmail.com>

Jul 2009, mk: Minor improvements
	- Saving output file as <in_file>_signed.sys automatically
	- Printing upload progress (it takes a while to upload large sis...)
'''

__version__ = '0.56'
__date__ = '15.08.2008'

import sys, os
from optparse import OptionParser
import ConfigParser
try:
    import wxversion
except ImportError:
    sys.exit('error: you need to install wxPython first')
wxversion.select('2.8')
import wx
import urllib, urllib2, cookielib
import mimetools, mimetypes
import poplib, rfc822, time
import tempfile
import time

capabilities = {'LocalServices': 14, 'Location': 17, 'NetworkServices': 13,
    'PowerMgmt': 2, 'ProtServ': 8, 'ReadDeviceData': 4, 'ReadUserData': 15,
    'SurroundingsDD': 18, 'SwEvent': 12, 'TrustedUI': 7, 'UserEnvironment': 19,
    'WriteDeviceData': 5, 'WriteUserData': 16}

class Callable:
    def __init__(self, anycallable):
        self.__call__ = anycallable

class MultipartPostHandler(urllib2.AbstractHTTPHandler): #BaseHandler
    '''Enables the use of multipart/form-data for posting forms
    
    02/2006 Will Holcomb <wholcomb@gmail.com>
    http://odin.himinbi.org/MultipartPostHandler.py
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
     
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    07/2008 Arkadiusz Wahlig <arkadiusz.wahlig@gmail.com>
    Improved to properly handle files and doseq=1 data at the same time.
    '''
    # needs to run first
    handler_order = urllib2.HTTPHandler.handler_order - 10
    # Controls how sequences are uncoded. If true, elements may be given
    # multiple values by assigning a sequence.
    doseq = True

    def http_request(self, request):
        data = request.get_data()
        if isinstance(data, dict):
            v_files = []
            v_vars = []
            try:
                 for key, value  in data.items():
                     if isinstance(value, file):
                         v_files.append((key, value))
                     else:
                         v_vars.append((key, value))
            except TypeError:
                systype, value, traceback = sys.exc_info()
                raise TypeError, "not a valid non-string sequence or mapping object", traceback

            if len(v_files) == 0:
                data = urllib.urlencode(v_vars, self.doseq)
            else:
                boundary, data = self.multipart_encode(v_vars, v_files)
                contenttype = 'multipart/form-data; boundary=%s' % boundary
                if (request.has_header('Content-Type')
                   and request.get_header('Content-Type').find('multipart/form-data') != 0):
                    print "Replacing %s with %s" % (request.get_header('content-type'), 'multipart/form-data')
                request.add_unredirected_header('Content-Type', contenttype)

            request.add_data(data)
        return request

    def multipart_encode(vars, files, boundary=None, buffer=None):
        if boundary is None:
            boundary = mimetools.choose_boundary()
        if buffer is None:
            buffer = ''
        for key, value in vars:
            if not isinstance(value, (list, tuple)):
                value = (value,)
            for v in value:
                buffer += '--%s\r\n' % boundary
                buffer += 'Content-Disposition: form-data; name="%s"' % key
                buffer += '\r\n\r\n' + str(v) + '\r\n'
        for key, fd in files:
            file_size = os.fstat(fd.fileno()).st_size
            filename = os.path.basename(fd.name)
            contenttype = mimetypes.guess_type(filename)[0] or 'application/octet-stream'
            buffer += '--%s\r\n' % boundary
            buffer += 'Content-Disposition: form-data; name="%s"; filename="%s"\r\n' % (key, filename)
            buffer += 'Content-Type: %s\r\n' % contenttype
            # buffer += 'Content-Length: %s\r\n' % file_size
            fd.seek(0)
            buffer += '\r\n' + fd.read() + '\r\n'
        buffer += '--%s--\r\n\r\n' % boundary
        return boundary, buffer
    multipart_encode = Callable(multipart_encode)

    #mk
    def https_open( self, req ):
	import httplib

	#self.set_http_debuglevel(1)
	
	class MyHTTPSConnection(httplib.HTTPSConnection):
	    def _sendall(self, stuff, flags = 0):
		totalLen = len(stuff)
		
		# Skip progress bar
		if totalLen < 0x10000:
		    return self.sock._ssl.write(stuff)

		# Sending with progress bar printed..
		totalSent = 0
		print "Upload progress: (total size %d bytes): [..........]\r" % (totalLen),
		print "Upload progress: (total size %d bytes): [" % (totalLen),
		while totalSent < totalLen:
		    chunkSize = min( totalLen/10+1, totalLen - totalSent )
		    chunk = stuff[totalSent:totalSent+chunkSize]
        	    totalSent += self.sock._ssl.write(chunk)
		    sys.stdout.write( "x" )
	        print ""
		return totalSent

	    def connect(self):
	        httplib.HTTPSConnection.connect(self)
		self.sock.sendall = self._sendall

        return self.do_open(MyHTTPSConnection, req)

    https_request = http_request

class CaptchaDialog(wx.Dialog):
    '''Simple wxPython dialog for asking user to solve a captcha.
    '''
    
    def __init__(self, parent, title, codeimage, text, *args, **kwds):
        self.codeimage = codeimage
        self.code = ''

        kwds["style"] = wx.DEFAULT_DIALOG_STYLE
        wx.Dialog.__init__(self, parent, *args, **kwds)
        self.label = wx.StaticText(self, -1, text)
        self.text = wx.TextCtrl(self, -1, '', style=wx.TE_PROCESS_ENTER)

        self.SetTitle(title)
        self.text.SetFocus()

        sizer_4 = wx.BoxSizer(wx.VERTICAL)
        sizer_1 = wx.BoxSizer(wx.HORIZONTAL)
        code = wx.StaticBitmap(self, -1, wx.Bitmap(self.codeimage, wx.BITMAP_TYPE_ANY), style=wx.STATIC_BORDER)
        sizer_4.Add(code, 0, wx.ALL, 8)
        sizer_1.Add(self.label, 0, wx.LEFT|wx.ALIGN_CENTER_VERTICAL, 8)
        sizer_4.Add(sizer_1, 1, wx.ALIGN_CENTER_HORIZONTAL, 0)
        sizer_4.Add(self.text, 0, wx.ALL|wx.EXPAND, 8)
        self.SetSizer(sizer_4)
        sizer_4.Fit(self)
        self.Layout()
        
        self.Center()

        self.Bind(wx.EVT_TEXT_ENTER, self.on_text_enter, self.text)

        self.SetReturnCode(wx.ID_CANCEL)

    def on_text_enter(self, event):
        self.code = self.text.GetValue().encode('utf8')
        self.EndModal(wx.ID_OK)

def parse_email(lines):
    '''Splits an email provided as a list of lines into a dictionary of
    header fields and a list of body lines. Returns a (header, body)
    tuple.
    '''
    part = 0
    head = {}
    body = []
    for ln in lines:
        if part == 0:
            if not ln.strip():
                part = 1
                continue
            i = ln.find(':')
            if i <= 0:
                continue
            head[ln[:i].strip().lower()] = ln[i+1:].strip()
        else:
            body.append(ln)
    return head, body

def warning(message):
    print >>sys.stderr, 'warning: %s' % message

def main():
    '''Main opensignsis processing function.
    '''   
    usage = 'usage: %prog [options] sis_input sis_output'
    version = '%%prog %s (%s)' % (__version__, __date__)
    
    # create parameters parser
    optparser = OptionParser(usage=usage, version=version)
    optparser.add_option('-i', '--imei', dest='imei',
        help='IMEI of the target device')
    optparser.add_option('-c', '--caps', dest='caps', metavar='CAPABILITIES',
        help='list of capabilities names, separated by +')
    optparser.add_option('-e', '--email', dest='email',
        help='e-mail address used to retrive the signed sis file')
    optparser.add_option('-s', '--server', dest='server', metavar='POP3_SERVER',
        help='host[:port] of the e-mail address POP3 server, defaults to the ' \
        'host part of the e-mail')
    optparser.add_option('-l', '--login', dest='login', metavar='LOGIN',
        help='POP3 server login name, defaults to the username part of the e-mail')
    optparser.add_option('-p', '--passwd', dest='passwd', metavar='PASSWORD',
        help='password associated with the login name, if ommited will cause ' \
        'a prompt at runtime')
    optparser.add_option('-t', '--ssl', dest='ssl', action='store_true',
        help='use SSL to login to POP3 server')
    optparser.add_option('-r', '--inplace', dest='inplace', action='store_true',
        help='replace the input file with the output file')
    optparser.add_option('-v', '--verbose', dest='verbose', action='store_true',
        help='print out more information while running')

    # parse config
    cfgparser = ConfigParser.SafeConfigParser()
    cfgoptions = {'device': ('imei',),
        'email': ('email', 'server', 'login', 'passwd', 'ssl')}
    cfgpath = os.path.join(os.path.dirname(__file__), 'opensignsis.config')
    if os.path.exists(cfgpath):
        # read config
        if cfgparser.read(cfgpath):
            for section, items in cfgoptions.items():
                for item in items:
                    try:
                        optparser.set_default(item, cfgparser.get(section, item))
                    except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
                        pass
    else:
        # write empty config file
        for section, items in cfgoptions.items():
            cfgparser.add_section(section)
            for item in items:
                cfgparser.set(section, item, '')
        try:
            h = open(cfgpath, 'w')
            cfgparser.write(h)
            h.close()
        except IOError, e:
            warning('couldn\'t create an empty config file (%s)' % e)

    # parse the parameters    
    options, args = optparser.parse_args()
    
    # validate params
    if len(args) == 0:
        optparser.error('specify input SIS file')
    sis_input = args[0]
    if len(args) == 1:
        if options.inplace:
            sis_output = sis_input
        else:
	    if( sis_input[-4:] == ".sis" ):
                sis_output = sis_input[:-4] + "_signed.sis"
	    else:
                optparser.error('specify output SIS file or -r/--inplace option')
    elif len(args) > 2:
        optparser.error('unexpected arguments')
    else:
        if options.inplace:
            optparser.error('remove -r/--inplace option if specifying an output SIS file')
        sis_output = args[1]

    # process caps option    
    usercaps = options.caps
    if usercaps is None:
        usercaps = 'ALL'
        warning('no caps specified, using %s' % repr(usercaps))
    if usercaps.upper() == 'ALL':
        usercaps = '+'.join(capabilities.keys())
    caps = []
    for c in usercaps.split('+'):
        cap = [x for x in capabilities.keys() if x.lower() == c.lower()]
        if not cap:
            optparser.error('Unknown capability: %s' % c)
        caps.append(cap[0])

    # validate params
    imei = options.imei
    if not imei:
        optparser.error('use -i/--imei option or add IMEI to the config file')

    print 'Using IMEI: %s' % imei

    # resolve sis_output params
    params = dict(imei=imei, caps='+'.join(caps))
    # convert all {x} params to lower-case
    low = sis_output.lower()
    for param in params.keys():
        pos = 0
        while True:
            pos = low.find('{%s}' % param, pos)
            if pos < 0:
                break
            sis_output = '%s{%s}%s' % (sis_output[:pos], param, sis_output[pos+len(param)+2:])
            pos += len(param)+2
    for param, value in params.items():
        sis_output = sis_output.replace('{%s}' % param, value)

    email = options.email
    if not email:
        optparser.error('use -e/--email option or add e-mail to the config file')

    # open input sis file
    sis_input_file = open(sis_input, 'rb')

    # create an URL opener with multipart POST and cookies support
    cookies = cookielib.CookieJar()
    opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cookies),
        MultipartPostHandler)

    codeimage = tempfile.mktemp('.jpg')

    wxapp = wx.PySimpleApp(0)
    wx.InitAllImageHandlers()

    print 'Processing security code...'

    try:
        while True:
            if options.verbose:
                print '* downloading image...'
        
            h = opener.open('https://www.symbiansigned.com/app/Captcha.jpg')
            data = h.read()
            h.close()
                    
            h = open(codeimage, 'wb')
            h.write(data)
            h.close()
            
            if options.verbose:
                print '* opening dialog...'
        
            dialog = CaptchaDialog(None, os.path.split(sis_input)[-1], codeimage,
                'Enter security code using only\nA-F letters and 0-9 numbers')
            result = dialog.ShowModal()
            if result != wx.ID_OK:
                sys.exit('Canceled')
            captcha = dialog.code.upper().replace('O', '0')
            dialog.Destroy()
        
            # if the code is empty, download a new one and repeat
            if captcha:
                break
    
            if options.verbose:
                print '* no code entered, retrying with a new one...'
    finally:
        try:
            os.remove(codeimage)
        except:
            pass

    if options.verbose:
        print '* security code: %s' % captcha
    
    # create request params
    params = dict(imei=imei,
        capabilities=[capabilities[x] for x in caps],
        email=email,
        application=sis_input_file,
        captcha=captcha,
        acceptLegalAgreement='true')

    print 'Sending request to Open Signed Online...'

    reqtime = time.time()
    h = opener.open('https://www.symbiansigned.com/app/page/public/openSignedOnline.do',
        params)
    data = h.read()
    h.close()

    # check for error
    pos = data.find('<td class="errorBox">')
    if pos >= 0:
        t = data[pos+21:]
        msg = t[:t.index('<br/>')].strip()
        
        sys.exit('error: %s' % msg)

    if options.verbose:
        print '* request sent'

    # validate options
    server = options.server
    if not server:
        server = email.split('@')[-1]
        warning('no POP3 server specified in args or in the config file, using %s' % repr(server))
    server = server.split(':')
    
    if options.ssl:
        POP3 = poplib.POP3_SSL
    else:
        POP3 = poplib.POP3
        
    login = options.login
    if not login:
        login = email.split('@')[0]
        warning('no login specified in args or in the config file, using %s' % repr(login))
        
    passwd = options.passwd
    if not passwd:
        warning('no password specified in args or in the config file, prompting for one now')
        from getpass import getpass
        passwd = getpass('Password for %s: ' % repr(login))

    print 'Waiting for request confirmation...'

    confirmed = False
    while True:
        if options.verbose:
            print '* polling the mailbox...'

        try:
            mbox = POP3(*server)
            mbox.user(login)
            mbox.pass_(passwd)
        except poplib.error_proto, e:
            sys.exit('error: %s' % e)

        for m in mbox.list()[1]:
            idx = int(m.split()[0])
            # we use top(), not retr(), so that the 'read'-flag isn't set
            head, body = parse_email(mbox.top(idx, 8192)[1])
            try:
                if time.mktime(rfc822.parsedate(head['date'])) < reqtime:
                    continue
            except:
                pass
            if head.get('from', '').lower() == 'donotreply@symbiansigned.com':
                for ln in body:
                    if ln.startswith('Confirmation link: https://www.symbiansigned.com/app/page/public/confirmrequest.pub?code='):
                        break
                else:
                    print 'Unknown message: %s' % head['subject']
                    # next email
                    continue
                
                if options.verbose:
                    print '* confirming'

                # confirm
                h = opener.open(ln[19:])
                data = h.read()
                h.close()

                if os.path.split(sis_input)[-1] in data:
                    confirmed = True
                                
                    if options.verbose:
                        print '* request confirmed'

                    mbox.dele(idx)
        
        mbox.quit()

        if confirmed:
            break
            
        time.sleep(3.0)
    
    print 'Waiting for signed file...'
    
    downloaded = False
    text = 'Your application (%s)' % os.path.split(sis_input)[-1]
    while True:
        if options.verbose:
            print '* polling the mailbox...'

        try:
            mbox = POP3(*server)
            mbox.user(login)
            mbox.pass_(passwd)
        except poplib.error_proto, e:
            sys.exit('error: %s' % e)

        for m in mbox.list()[1]:
            idx = int(m.split()[0])
            # we use top(), not retr(), so that the 'read'-flag isn't set
            head, body = parse_email(mbox.top(idx, 8192)[1])
            try:
                if time.mktime(rfc822.parsedate(head['date'])) < reqtime:
                    continue
            except:
                pass
            if head.get('from', '').lower() == 'donotreply@symbiansigned.com':
                if len(body) == 0 or not text in body[0]:
                    continue

                mbox.dele(idx)

                for ln in body:
                    if ln.startswith('Download link: https://www.symbiansigned.com/app/page/public/downloadapplication.pub?code='):
                        break
                else:
                    if 'signing has failed' in ' '.join(body):
                        mbox.quit()
                        sys.exit('error: %s' % ' '.join(body).strip())

                    print 'Unknown message: %s' % head['subject']
                    # next email
                    continue
                
                if options.verbose:
                    print '* downloading sis file'

                # download
                h = opener.open(ln[15:])
                data = h.read()
                h.close()

                # save as output sis
                h = open(sis_output, 'wb')
                h.write(data)
                h.close()
                
                downloaded = True
                if options.verbose:
                    print '* saved as %s' % sis_output

                break

        mbox.quit()

        if downloaded:
            break
            
        time.sleep(6.0)

    print 'Saved as: %s' % sis_output

if __name__ == '__main__':
    sys.exit(main())
