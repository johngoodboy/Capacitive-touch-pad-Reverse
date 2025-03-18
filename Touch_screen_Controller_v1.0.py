#Начиная с версии 1.0 решил писать не отдельные функции, а описать окно на основе класса
#1.0 - это рабочая версия без кнопки для отправки команды на взлом пароля.

from PyQt5 import QtWidgets, uic
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo
from PyQt5.QtCore import QIODevice
from PyQt5.QtGui import QIcon

import sys


class Window(QtWidgets.QMainWindow):
    def __init__(self):#Конструктор
        super(Window, self).__init__() # Вызываем конструктор родительского класса
        uic.loadUi('Touch_screen_Controller_v1.0.ui', self) #Загружаем наш интерфейс, ктр создали в QTDesigner 
        
        self.serial = QSerialPort()#Создаём объект класса QSerialPort (экземпляр порта)
        #self.serialStrbuffer = QString()
        #Указываем, какие методы вызывать при событиях

        #Кнопки
        self.COMPort_btn.clicked.connect(self.BeginSerialCOMPort)
        self.PWR_ON_btn.clicked.connect(self.onPWR_ON_btnclick)
        self.UnBlock_btn.clicked.connect(self.onUnBlock_btnclick)
        self.RemoveStartScreen_btn.clicked.connect(self.onRemoveStartScreen_btnclick)

        self.btn_0.clicked.connect(self.onNumbtnclick)
        self.btn_1.clicked.connect(self.onNumbtnclick)
        self.btn_2.clicked.connect(self.onNumbtnclick)
        self.btn_3.clicked.connect(self.onNumbtnclick)
        self.btn_4.clicked.connect(self.onNumbtnclick)
        self.btn_5.clicked.connect(self.onNumbtnclick)
        self.btn_6.clicked.connect(self.onNumbtnclick)
        self.btn_7.clicked.connect(self.onNumbtnclick)
        self.btn_8.clicked.connect(self.onNumbtnclick)
        self.btn_9.clicked.connect(self.onNumbtnclick)

        #Com порт
        self.serial.readyRead.connect(self.onRead) 

        #Буфер для накапливания строк из ком порта.
        self.serialStrbuffer = "" 

    def BeginSerialCOMPort(self):
        btn_name = self.COMPort_btn.text()
        if btn_name =='Поиск портов':
            portlist = []
            ports = QSerialPortInfo().availablePorts()
            for port in ports:
                portlist.append(port.portName())
            if len(portlist) == 0:
                self.plainTextEditLog.appendPlainText("COM портов нет.")
            else:
                self.comboBox_COMPort.clear()#Удаляем записи портов с прошлого раза. Без этой очистки могут быть одинаковые порты.
                self.comboBox_COMPort.addItems(portlist)
                self.COMPort_btn.setText('Подключиться')
        elif btn_name =='Подключиться':#Если сом порты найдены
            self.serial.setPortName(self.comboBox_COMPort.currentText())
            self.serial.setBaudRate(115200)
            portOpened=self.serial.open(QIODevice.ReadWrite)
            if portOpened:#Если порт открылся (нет ошибок)
                text_str = "Подключились к порту " + self.comboBox_COMPort.currentText()
                self.plainTextEditLog.appendPlainText(text_str)
                self.COMPort_btn.setText('Отключиться')
        elif btn_name =='Отключиться':#Если подключены к порту
            self.COMPort_btn.setText('Поиск портов')
            self.serial.close()
            text_str = "Отключились от порта " + self.comboBox_COMPort.currentText()
            self.plainTextEditLog.appendPlainText(text_str)

    def onRead(self):
        rx_data = self.serial.readAll()#Считываем куски строк из буфера
        rx_str = str(rx_data, 'utf-8')# Переводим массив байт в строку
        #Соединяем куски в цельные строки в нашем буфере. Разделитель "", т е без разделителя. 
        self.serialStrbuffer = "".join([self.serialStrbuffer, rx_str])
        #print(self.serialStrbuffer)

        # Разделяем строки на подстроки на символе конца строки
        tempList = self.serialStrbuffer.split("\r\n")
        #print(tempList)

        #Как только приходит кусок строки, содержащий символ конца строки \r\n,
        # в списке становится на одну строку больше. Значит в первом элементе списка целая строка.
        #Иногда прилетает сразу 2 целых строки. Поэтому сделаем цикл. Последний элемент списка 
        # содержит символ конца строки.
        length = len(tempList)
        if length>1 and tempList[length-1] == '':# Если строки стало 2 и последняя пустая
            
            #for string in tempList:
                #self.plainTextEditLog.appendPlainText(string)
            #Выводим все строки из списка, кроме последней.
            for i in range(0,length-1,1):
                self.plainTextEditLog.appendPlainText(tempList[i])

            self.serialStrbuffer = ""#очищаем буфер
            #  очищаем список, (он и так очистится, т к очистили буфер)

        
    
    def onPWR_ON_btnclick(self):#Обработчик нажатия на кнопку включения телефона
        command = bytes([0x20,0x00,0x00,0x00])#Команда 20 - включить Телефон
        self.serial.write(command)
        self.plainTextEditLog.appendPlainText("Послали команду на включение телефона")

    def onUnBlock_btnclick(self):#Обработчик нажатия на кнопку блокировки телефона
        command = bytes([0x30,0x00,0x00,0x00])#Команда 30 - разблокировать Телефон
        self.serial.write(command)
        self.plainTextEditLog.appendPlainText("Послали команду на разблокирование телефона")

    def onRemoveStartScreen_btnclick(self):#Обработчик нажатия на кнопку смах заставки
        #Команда 40 - смах заставки. Начальная точка X=06F, Y=35E
        command = bytes([0x40,0x03,0x6F,0x5E])
        self.serial.write(command)
        self.plainTextEditLog.appendPlainText("Послали команду на смах заставки")

    def onNumbtnclick(self):#Обработчик нажатий на кнопки 0-9
        btn = self.sender()#Метод sender() возвращает Qt объект, который посылает сигнал.
        text = btn.text()

        #self.plainTextEditLog.appendPlainText(btn.text())#Выводим цифру в информационное поле
        #Здесь нужно ещё проверять, подсоединены ли мы к имитатору по юарт. Открыт ли порт.
        #Если нет, то выводить сообщение.
        if text == '0':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 0
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 0 X=120, Y=2E1
            xy = bytes([0x10, 0x12,0x20,0xE1])#Команда 10 - сделать касание
            self.serial.write(xy)
            #print(xy)
            #print(xy[0])

        elif text == '1':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 1
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 1 X=080, Y=162 
            xy = bytes([0x10, 0x01,0x80,0x62])
            self.serial.write(xy)
        elif text == '2':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 2
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 2 X=120, Y=162
            xy = bytes([0x10, 0x11,0x20,0x62])
            self.serial.write(xy)
        elif text == '3':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 3
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 3 X=1B4, Y=162
            xy = bytes([0x10, 0x11,0xB4,0x62])
            self.serial.write(xy)
        elif text == '4':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 4
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 4 X=080, Y=1E3
            xy = bytes([0x10, 0x01,0x80,0xE3])
            self.serial.write(xy)
        elif text == '5':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 5
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 5 X=120, Y=1E3
            xy = bytes([0x10, 0x11,0x20,0xE3])
            self.serial.write(xy)
        elif text == '6':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 6
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 6 X=1B4, Y=1E3
            xy = bytes([0x10, 0x11,0xB4,0xE3])
            self.serial.write(xy)
        elif text == '7':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 7
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 7 X=080, Y=262
            xy = bytes([0x10, 0x02,0x80,0x62])
            self.serial.write(xy)
        elif text == '8':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 8
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 8 X=120, Y=262
            xy = bytes([0x10, 0x12,0x20,0x62])
            self.serial.write(xy)
        elif text == '9':
            #Отправляем имитатору касаний координаты для касания парольной клавиши 9
            #Формат такой:Команда, XHYH, XL, YL.
            #Для 9 X=1B4, Y=262
            xy = bytes([0x10, 0x12,0xB4,0x62])
            self.serial.write(xy)
        
    
#Проверяем, запускается ли наш код как код верхнего уровня или нет.
#Если будем запускать данный файл как основной, то иф выполнится и окно покажется. 
#Если будем импортировать данный файл (или какой-то класс из него) в какой-то другой модуль,
#то окно не вызовется. Но ведь нам это и не нужно.
if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    window = Window()#Создаём экземпляр класса.
    window.show()
    #app.exec()
    sys.exit(app.exec())