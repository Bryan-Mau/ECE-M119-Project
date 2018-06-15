import bluepy.btle
from bluepy.btle import Scanner, DefaultDelegate, ScanEntry, Peripheral
import struct
import time

waitingForResponse1 = True
waitingForResponse2 = True
challengeAccepted1 = False
challengeAccepted2 = False

rescan = False
choice1 = 0
choice2 = 0
#This is a delegate for receiving BTLE events
class BTEventHandler(DefaultDelegate):
	def __init__(self, id):
		DefaultDelegate.__init__(self)
		self.id = id

	def handleDiscovery(self, dev, isNewDev, isNewData):
		# Advertisement Data
		if isNewDev:
			print "Found new device:", dev.addr, dev.getScanData()

		# Scan Response
		if isNewData:
			print "Received more data", dev.addr, dev.getScanData()

	def handleNotification(self, cHandle, data):
		# Only print the value when the handle is 40 (the battery characteristic)
		if cHandle == 40:
			received = struct.unpack('B', data)
			if self.id == 1:
				global waitingForResponse1
				global challengeAccepted1
				global choice1
				if received[0] == 1:
					print "rock"
					choice1 = 1
					waitingForResponse1 = False
				if received[0] == 2:
					print "paper"
					choice1 = 2
					waitingForResponse1 = False
				if received[0] == 3:
					print "scissors"
					choice1 = 3
					waitingForResponse1 = False
				if received[0] == 4:
					print "Challenge accepted by player 1"
					waitingForResponse1 = False
					challengeAccepted1 = True
				if received[0] == 5:
					print "Challenge rejected by player 1"
					waitingForResponse1 = False
					challengeAccepted1 = False
			else:
				global waitingForResponse2
				global challengeAccepted2
				global choice2
				if received[0] == 1:
					print "rock"
					choice2 = 1
					waitingForResponse2 = False
				if received[0] == 2:
					print "paper"
					choice2 = 2
					waitingForResponse2 = False
				if received[0] == 3:
					print "scissors"
					choice2 = 3
					waitingForResponse2 = False
				if received[0] == 4:
					print "Challenge accepted by player 2"
					waitingForResponse2 = False
					challengeAccepted2 = True
				if received[0] == 5:
					print "Challenge rejected by player 2"
					waitingForResponse2 = False
					challengeAccepted2 = False
			
def try_until_success(func, exception=bluepy.btle.BTLEException, msg='reattempting', args=[]):
	retry = True
	while True:
		try:
			func(*args)
			retry = False
		except exception:
			print msg
			
		if not retry:
			break

def determine_winner(c1, c2):
	if c1 == 1:
		if c2 == 1:
			return 0
		elif c2 == 2:
			return 2
		elif c2 == 3:
			return 1
	elif c1 == 2:
		if c2 == 1:
			return 1
		elif c2 == 2:
			return 0
		elif c2 == 3:
			return 2
	elif c1 == 3:
		if c2 == 1:
			return 2
		elif c2 == 2:
			return 1
		elif c2 == 3:
			return 0

def send_to_hexi(output, package):
	curr_len = len(package)
	padding_size = 20-curr_len
	padding = ""
	i = 0
	while i < padding_size:
		padding = padding+"0"
		i+=1
	new_package = package+padding 
	try_until_success(output.write, msg="Failed to send to player 1", args=[new_package,True])

handler1 = BTEventHandler(1)
handler2 = BTEventHandler(2)

with open("mac.txt") as f:
	macAddresses = f.readlines()
macAddresses = [m.strip() for m in macAddresses]

# Create a scanner with the handler as delegate
scanner1 = Scanner().withDelegate(handler1)
scanner1.clear()
while True:
	while True:
		# Start scanning. While scanning, handleDiscovery will be called whenever a new device or new data is found
		devs = scanner1.scan(3)

		# Get HEXIWEAR's address
		devices = [dev for dev in devs if dev.getValueText(0x8) == 'HEXIWEAR']
		hexi_addr = []
		for dev in devices:
			print dev.addr
			if dev.addr in macAddresses:
				hexi_addr.append(dev.addr)
		print hexi_addr
		if len(hexi_addr) >= 2:
			player1 = hexi_addr[0]
			player2 = hexi_addr[1]
			break
		time.sleep(5)

	print("Found challenger")
	# Create a Peripheral object with the delegate
	hexi1 = Peripheral().withDelegate(handler1)
	hexi2 = Peripheral().withDelegate(handler2)
	# Connect to Hexiwear
	try_until_success(hexi1.connect, msg = "Failed Connection1", args = [player1])
	print "Connected to player 1"
	try_until_success(hexi2.connect, msg = "Failed Connection2", args = [player2])
	print "Connected to player 2"

	# Get the services
	battery1 = hexi1.getCharacteristics(uuid="2a19")[0]
	battery2 = hexi2.getCharacteristics(uuid="2a19")[0]
	temp1 = hexi1.getCharacteristics(uuid="2012")[0]
	temp2 = hexi2.getCharacteristics(uuid="2012")[0]
	humidity1 = hexi1.getCharacteristics(uuid="2013")[0]
	humidity2 = hexi1.getCharacteristics(uuid="2013")[0]
	pressure1 = hexi1.getCharacteristics(uuid="2014")[0]
	pressure2 = hexi1.getCharacteristics(uuid="2014")[0]

	# Get the client configuration descriptor and write 1 to it to enable notification
	battery_desc1 = battery1.getDescriptors(forUUID=0x2902)[0]
	battery_desc2 = battery2.getDescriptors(forUUID=0x2902)[0]
	battery_desc1.write(b"\x01", True)
	battery_desc2.write(b"\x01", True)
	output1 = hexi1.getCharacteristics(uuid='2031')[0]
	output2 = hexi2.getCharacteristics(uuid='2031')[0]

	# Infinite loop to receive notifications
	while True:
		#unpack returns a tuple (x,)
		t1 = struct.unpack('h',temp1.read())
		t2 = struct.unpack('h',temp2.read())
		print t1[0]
		print t2[0]
		time.sleep(1)
		h1 = struct.unpack('h',humidity1.read())
		h2 = struct.unpack('h',humidity2.read())
		p1 = struct.unpack('h',pressure1.read())
		p2 = struct.unpack('h',pressure2.read())
		name1_1 = chr(h1[0]>>8)
		name1_2 = chr(h1[0]&0b0000000011111111)
		name1_3 = chr(p1[0]>>8)
		name1_4 = chr(p1[0]&0b0000000011111111)
		name2_1 = chr(h2[0]>>8)
		name2_2 = chr(h2[0]&0b0000000011111111)
		name2_3 = chr(p2[0]>>8)
		name2_4 = chr(p2[0]&0b0000000011111111)
		name1 = name1_1+name1_2+name1_3+name1_4+'0000000000000000'
		name2 = name2_1+name2_2+name2_3+name2_4+'0000000000000000'
		print("Sending challengers' names")
		send_to_hexi(output1, name2)
		send_to_hexi(output2, name1)
        while True:
			while True:
				if(waitingForResponse1 or waitingForResponse2):
					hexi1.waitForNotifications(0.5)
					hexi2.waitForNotifications(0.5)
					time.sleep(1)
				else:
					waitingForResponse1 = True
					waitingForResponse2 = True
					break

			if(not(challengeAccepted1 or challengeAccepted2)):
				challengeAccepted1 = False
				challengeAccepted2 = False
				rescan = True
				break
			challengeAccepted1 = False
			challengeAccepted2 = False
			print "Starting Game"
			send_to_hexi(output1, "a")
			send_to_hexi(output2, "a")
			while True:
				if(waitingForResponse1 or waitingForResponse2):
					hexi1.waitForNotifications(0.5)
					hexi2.waitForNotifications(0.5)
					time.sleep(1)
				else:
					waitingForResponse1 = True
					waitingForResponse2 = True
					break
			winner = determine_winner(choice1, choice2)
			choice1 = 0
			choice2 = 0
			if winner == 0:
				print "Game was drawn"
				send_to_hexi(output1, "t")
				send_to_hexi(output2, "t")
			elif winner == 1:
				print "Player 1 has won"
				send_to_hexi(output1, "w")
				send_to_hexi(output2, "l")
			else:
				print "Player 2 has won"
				send_to_hexi(output1, "l")
				send_to_hexi(output2, "w")
		if rescan
			hexi1.disconnect()
			hexi2.disconnect()
			rescan = False
			break
	print "rescan"
        


