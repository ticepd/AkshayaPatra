"""
    Author : Rahul Sharma
    CEDT NSIT
"""

import sys
from PyQt5 import QtCore, QtWidgets, QtGui, uic
import glob
import serial
import time
import os, platform

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
        self.ProgramButton.clicked.connect(self.JTAG)
        self.textEdit.setReadOnly(True)
        self.FileNameText.setReadOnly(True)
        #Label        
        self.label_5.setText('''<a href='http://cedtnsit.in/'>CEDT NSIT</a>''')
        self.label_5.setOpenExternalLinks(True)
        self.label_3.setPixmap(QtGui.QPixmap("logo1.png"))
        self.label_3.setFixedSize(QtGui.QPixmap("logo1.png").size())
        
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
        self.fname, _ = QtWidgets.QFileDialog.getOpenFileName(self, 'Open file','',"*.xsvf")
        if self.fname != '' :
            self.FileNameText.setText(self.fname)   
            self.getFileSize()

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
            self.getFileSize()
        else:
            e.ignore()    
        
    def ComPORT(self) :
        self.ComPort = self.ComPortDrop.currentText()

    def JTAG(self) :
        self.ComPort = self.ComPortDrop.currentText()
        self.runJTAG(self.ComPort,self.fname)
    def getFileSize(self) :
        filesize=0
        try:
            self.textEdit.append('Opening source file...')
            xsvf=open(self.fname,'rb')
            ch=xsvf.read(1)
            while ch!="":
                filesize=filesize+1
                ch=xsvf.read(1)
            xsvf.close()
            self.textEdit.append('File successfully opened.')
            self.textEdit.append('File size : {} bytes'.format(filesize) )
            self.progressBar.setMaximum(filesize)
        except:
            self.textEdit.append('Error in opening file. No such file exists!')
            exit()        

    def runJTAG(self, cp, fn) :
        #----------------------------------------------------------
        #Serial port specification
        baud=250000
        port=cp
        #----------------------------------------------------------
        #Location of .xsvf file to be programmed
        filename=fn
        #----------------------------------------------------------
        #Function to return number of bytes reuiqred by 'argument' bits
        def numberOfBytes(argument):
            temp=int(argument.encode('hex'),16)#Find Decimal equivalent of 4 bytes
            return (int)((temp+7)>>3)          #Number of bytes required to represent temp
            #----------------------------------------------------------
            #Function to return how many bytes to send for the instruction
        def bytesToSend(instruction):
            global sdr_bytes
            global ir_size
            global flag
            command=instruction.encode('hex')
            if command=="00":   #XCOMPLETE
                ser.write(instruction)
                self.textEdit.append("Reached End Of File. Operation Successful")
            elif command=="01": #XTDOMASK
                ser.write(instruction)
                nextBytes=xsvf.read(sdr_bytes)  #Read sdr_bytes specified by last XSDRSIZE instruction        
                ser.write(nextBytes)
            elif command=="02": #XSIR
                ser.write(instruction)
                nextBytes=xsvf.read(1)          #Read a single bytes to find ir_size(bytes)
                ser.write(chr(0x0))
                ser.write(chr(0x0))
                ser.write(chr(0x0))
                ser.write(nextBytes)
                ir_size=numberOfBytes(nextBytes)#Return bytes required by IR
                nextBytes=xsvf.read(ir_size)    #Reads bytes equivalent to ir_size
                ser.write(nextBytes)
            elif command=="04": #XRUNTEST
                ser.write(instruction)
                nextBytes=xsvf.read(4)          #Read 4 bytes to find runtest time
                ser.write(nextBytes)
            elif command=="07": #XREPEAT
                ser.write(instruction)
                nextBytes=xsvf.read(1)
                ser.write(nextBytes)
            elif command=="08": #XSDRSIZE
                ser.write(instruction)
                nextBytes=xsvf.read(4)              #Read 4 bytes
                sdr_bytes=numberOfBytes(nextBytes)  #Return bytes required by SDR
                ser.write(nextBytes)           #Send sdr_bytes, 1 byte, chr() converts int to char
            elif command=="09": #XSDRTDO    
                ser.write(instruction)
                nextBytes=xsvf.read(sdr_bytes)  #Read TDIValue                
                ser.write(nextBytes)       
                nextBytes=xsvf.read(sdr_bytes)  #Reads TDOExpected 
                ser.write(nextBytes)
            elif command=="12": #XSTATE
                ser.write(instruction)
                nextBytes=xsvf.read(1)
                ser.write(nextBytes)            #Reads the TAP State Description  
                #----------------------------------------------------------
                #Main Program starts here
        try:
            filename=fn
            try:
                self.textEdit.append('Establishing connection to programmer....')
                ser=serial.Serial(port,baud)
                time.sleep(2)
                self.textEdit.append('Connected to programmer.')
            except:
            #If error in connecting to port, stop program execution
                self.textEdit.append('Error in connecting to programmer!')
                exit()   
            start_time=time.time()
            self.textEdit.append('Programming Target. Please Wait...')
            xsvf=open(filename,'rb')
            ser.flush()
            incomingByte=ser.read()
            #Programmer send 'S' when initialized and after successful operation else 'F' 
            if incomingByte=='S':
                byteRead=xsvf.read(1)
                while byteRead!="":
                    bytesToSend(byteRead)
                    incomingByte=ser.read()
                    if incomingByte!='S':
                        break
                    byteRead=xsvf.read(1)   
                    self.progressBar.setValue(xsvf.tell())
                    QtWidgets.QApplication.processEvents()
            self.textEdit.append(str((time.time() - start_time))+"seconds")  
            ser.close()
            xsvf.close()
        except:
            self.textEdit.append('Unsuccessful Operation! Reconnect everything and try again.')
        
        
if __name__ == "__main__" :
    app = QtWidgets.QApplication(sys.argv)
    window = MyApp()
    window.show()
    sys.exit(app.exec_())
