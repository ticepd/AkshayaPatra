import sys, os, platform
from PyQt5 import QtCore, QtWidgets, QtGui, uic
import glob
import serial
import time

op_sys = platform.system()
if op_sys == 'Darwin':
    from Foundation import NSURL

qtCreaterFile = "JtagUI.ui"

Ui_MainWindow, QtBaseClass = uic.loadUiType(qtCreaterFile)

class MyApp(QtWidgets.QMainWindow, Ui_MainWindow) :
    def __init__(self) :
        QtWidgets.QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)
        self.GetFileButton.clicked.connect(self.filePath)
        self.serial_ports()
        self.ComPortDrop.addItems(self.result)
        self.ComPortDrop.activated.connect(self.ComPORT)
        self.BaudDrop.addItems(['4800','9600','19200','38400','57600','115200','230400',\
                                '250000','460800','921600','1500000'])
        self.BaudDrop.activated.connect(self.BaudSet)
        self.ProgramButton.clicked.connect(self.JTAG)
        self.textEdit.setReadOnly(True)
        self.FileNameText.setReadOnly(True)
        #Label        
        self.label_5.setText('''<a href='http://cedtnsit.in/'>CEDT NSIT</a>''')
        self.label_5.setOpenExternalLinks(True)

    def serial_ports(self):
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')
                        
        self.result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                self.result.append(port)
            except (OSError, serial.SerialException):
                pass

    def filePath(self) :
        self.fname, _ = QtWidgets.QFileDialog.getOpenFileName(self, 'Open file',"*.xsvf")
        self.FileNameText.setText(self.fname)   
        #print self.fname
        
    def dragEnterEvent(self, e):
        if e.mimeData().hasUrls:
            e.accept()
        else:
            e.ignore()

    def dragMoveEvent(self, e):
        if e.mimeData().hasUrls:
            e.accept()
        else:
            e.ignore()

    def dropEvent(self, e):
        """
        Drop files directly onto the widget
        File locations are stored in fname
        :param e:
        :return:
        """
        if e.mimeData().hasUrls:
            e.setDropAction(QtCore.Qt.CopyAction)
            e.accept()
            # Workaround for OSx dragging and dropping
            for url in e.mimeData().urls():
                if op_sys == 'Darwin':
                    fname = str(NSURL.URLWithString_(str(url.toString())).filePathURL().path())
                else:
                    fname = str(url.toLocalFile())

            self.fname = fname
            self.FileNameText.setText(self.fname)
            #self.load_image()
        else:
            e.ignore()          
        
    def ComPORT(self) :
        self.ComPort = self.ComPortDrop.currentText()
            
    def BaudSet(self) :
        self.baudrate = int(self.BaudDrop.currentText())
        

    def numberOfBytes(self, argument):
        temp=int(argument.encode('hex'),16)
        return (int)((temp+7)>>3)
        
    def bytesToSend(self):
        self.sdr_bytes
        self.ir_size
        command=self.byteRead.encode('hex')
        print command
        print self.byteRead
        if command=="00":   #XCOMPLETE
            self.ser.write(self.byteRead)
            self.textEdit.append('Reached end of File')
            self.textEdit.setTextColor(QtGui.QColor("green"))
            self.textEdit.setFontPointSize(15)
            self.textEdit.append("Operation Successful")
        elif command=="01": #XTDOMASK
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(self.sdr_bytes)  
            self.ser.write(nextBytes)
        elif command=="02": #XSIR
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(1)          
            self.ser.write(chr(0x0))
            self.ser.write(chr(0x0))
            self.ser.write(chr(0x0))
            self.ser.write(nextBytes)
            self.ir_size=self.numberOfBytes(nextBytes)
            nextBytes=self.xsvf.read(self.ir_size)    
            self.ser.write(nextBytes)
        elif command=="04": #XRUNTEST
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(4)          
            self.ser.write(nextBytes)
        elif command=="07": #XREPEAT
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(1)
            self.ser.write(nextBytes)
        elif command=="08": #XSDRSIZE
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(4)              
            self.sdr_bytes=self.numberOfBytes(nextBytes)  
            self.ser.write(nextBytes)           
        elif command=="09": #XSDRTDO    
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(self.sdr_bytes)             
            self.ser.write(nextBytes)       
            nextBytes=self.xsvf.read(self.sdr_bytes)  
            self.ser.write(nextBytes)
        elif command=="12": #XSTATE
            self.ser.write(self.byteRead)
            nextBytes=self.xsvf.read(1)
            self.ser.write(nextBytes) 
        
    def JTAG(self) :
        self.textEdit.append('Programming')
        try:
            try:
                self.textEdit.append('Opening source file...')
                #Open File in read+binary mode
                self.xsvf = open(self.fname, 'rb')         
                statinfo = os.stat(self.fname)
                self.xsvf.close()
                self.xsvf = open(self.fname, 'rb')
                #self.textEdit.append(str(statinfo.st_size))
                self.textEdit.append('File successfully opened.')
                self.textEdit.append('File size : {} bytes'.format(statinfo.st_size))
            except :
                #If error in opening file, stop program execution
                self.textEdit.append('Error in opening file. No such file exists!')
                exit()
                
            try:
                self.ComPort = self.ComPortDrop.currentText()
                self.baudrate = int(self.BaudDrop.currentText())
                self.textEdit.append('Establishing connection to programmer....')
                self.ser=serial.Serial(self.ComPort,self.baudrate)
                time.sleep(2)
                self.textEdit.append('Connected to programmer.')
            except:
                #If error in connecting to port, stop program execution
                self.textEdit.append('Error in connecting to programmer!')
                exit()
            i=0
            #Parse .xsvf file and pump the bytes
            start_time=time.time()
            self.textEdit.append('Programming Target. Please Wait...')
            self.ser.flush()
            incomingByte=self.ser.read()
            #Programmer send 'S' when initialized and after successful operation else 'F'    
            if incomingByte=='S':
                self.byteRead=self.xsvf.read(1)
                while self.byteRead!="":
                    self.bytesToSend()
                    incomingByte=self.ser.read()                    
                    print incomingByte
                    if incomingByte!='S':
                        break
                    self.byteRead=self.xsvf.read(1)   
                    i = i+1                    
                    print '[%3d %%]\r'%(i*100/5504),
            #print incomingByte
            #print self.ser.read()
            self.textEdit.append((time.time() - start_time)-15,"seconds")                                                              
            self.ser.close()
            self.xsvf.close()
        except:
            self.textEdit.setTextColor(QtGui.QColor("red"))
            self.textEdit.setFontPointSize(12)
            self.textEdit.append('\nUnsuccessful Operation!')
         
        
if __name__ == "__main__" :
    app = QtWidgets.QApplication(sys.argv)
    window = MyApp()
    window.show()
    sys.exit(app.exec_())