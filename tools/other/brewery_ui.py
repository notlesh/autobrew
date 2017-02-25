#!/usr/bin/env python

# non-web based UI for brewery
# no graphs, either. meant to be lightweight.

import Tkinter as tk
import urllib2
import tkFont

class Application(tk.Frame):
	def __init__(self, master=None):
		tk.Frame.__init__(self, master)

		self.grid()
		self.createWidgets()

	def createWidgets(self):

		medFont = tkFont.Font(family="DejaVuSans", size=14)
		bigFont = tkFont.Font(family="DejaVuSans", size=16, weight=tkFont.BOLD)

		# PUMP 1
		self.p1Frame = tk.LabelFrame(self, text="Pump 1", padx=5, pady=5, font=medFont)
		# self.p1Frame.pack(fill="both", expand="yes", padx=10, pady=10)
		self.p1Frame.grid(row=0, column=0)

		self.p1OnButton = tk.Button(
				self.p1Frame,
				text='Pump 1 On',
				command=self.sendP1OnRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.p1OnButton.pack(padx=4, pady=4)
		self.p1OffButton = tk.Button(
				self.p1Frame,
				text='Pump 1 Off',
				command=self.sendP1OffRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.p1OffButton.pack(padx=4, pady=4)

		# PUMP 2
		self.p2Frame = tk.LabelFrame(self, text="Pump 2", padx=5, pady=5, font=medFont)
		# self.p2Frame.pack(fill="both", expand="yes", padx=10, pady=10)
		self.p2Frame.grid(row=0, column=1)

		self.p2OnButton = tk.Button(
				self.p2Frame,
				text='Pump 2 On',
				command=self.sendP2OnRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.p2OnButton.pack(padx=4, pady=4)
		self.p2OffButton = tk.Button(
				self.p2Frame,
				text='Pump 2 Off',
				command=self.sendP2OffRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.p2OffButton.pack(padx=4, pady=4)

		# VALVES
		self.valveFrame = tk.LabelFrame(self, text="Valve", padx=5, pady=5, font=medFont)
		# self.valveFrame.pack(fill="both", expand="yes", padx=10, pady=10)
		self.valveFrame.grid(row=1, column=0, columnspan=2)

		self.valveOnButton = tk.Button(
				self.valveFrame,
				text='Valve On',
				command=self.sendValveOnRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.valveOnButton.pack(padx=4, pady=4)
		self.valveOffButton = tk.Button(
				self.valveFrame,
				text='Valve Off',
				command=self.sendValveOffRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.valveOffButton.pack(padx=4, pady=4)
		self.valveFloatButton = tk.Button(
				self.valveFrame,
				text='Valve Float',
				command=self.sendValveFloatRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.valveFloatButton.pack(padx=4, pady=4)

		# BK
		self.bkFrame = tk.LabelFrame(self, text="BK Element", padx=5, pady=5, font=medFont)
		# self.bkFrame.pack(fill="both", expand="yes", padx=10, pady=10)
		self.bkFrame.grid(row=2, column=0)

		'''
		self.bkOnButton = tk.Button(
				self.bkFrame,
				text='BK On',
				command=self.sendBkOnRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.bkOnButton.pack(padx=4, pady=4)
		'''
		self.bkOffButton = tk.Button(
				self.bkFrame,
				text='BK Off',
				command=self.sendBkOffRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.bkOffButton.pack(padx=4, pady=4)

		# HLT
		self.hltFrame = tk.LabelFrame(self, text="HLT Element", padx=5, pady=5, font=medFont)
		# self.hltFrame.pack(fill="both", expand="yes", padx=10, pady=10)
		self.hltFrame.grid(row=2, column=1)

		'''
		self.hltOnButton = tk.Button(
				self.hltFrame,
				text='HLT On',
				command=self.sendHltOnRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.hltOnButton.pack(padx=4, pady=4)
		'''
		self.hltOffButton = tk.Button(
				self.hltFrame,
				text='HLT Off',
				command=self.sendHltOffRequest,
				padx=20,
				pady=12,
				font=bigFont)
		self.hltOffButton.pack(padx=4, pady=4)

		'''
		self.p2OnButton = tk.Button(self, text='Pump 2 on', command=self.sendP2OnRequest)
		self.p2OnButton.grid()
		self.p2OffButton = tk.Button(self, text='Pump 2 off', command=self.sendP2OffRequest)
		self.p2OffButton.grid()

		self.valveOnButton = tk.Button(self, text='Valve on', command=self.sendValveOnRequest)
		self.valveOnButton.grid()
		self.valveOffButton = tk.Button(self, text='Valve off', command=self.sendValveOffRequest)
		self.valveOffButton.grid()
		self.valveFloatButton = tk.Button(self, text='Valve float', command=self.sendValveFloatRequest)
		self.valveFloatButton.grid()

		self.bkOffButton = tk.Button(self, text='BK Off', command=self.sendBkOffRequest)
		self.bkOffButton.grid()

		self.hltOffButton = tk.Button(self, text='HLT Off', command=self.sendHltOffRequest)
		self.hltOffButton.grid()

		'''

		self.quitButton = tk.Button(self, text='Quit', command=self.quit)
		# self.quitButton.pack(padx=4, pady=4)
		self.quitButton.grid(columnspan=2)

	def sendRequest(self, url):
		urllib2.urlopen("http://localhost" + url)

	def sendP1OnRequest(self):
		self.sendRequest(url="/ab?cmd=p1_on")
	def sendP1OffRequest(self):
		self.sendRequest(url="/ab?cmd=p1_off")

	def sendP2OnRequest(self):
		self.sendRequest(url="/ab?cmd=p2_on")
	def sendP2OffRequest(self):
		self.sendRequest(url="/ab?cmd=p2_off")

	def sendValveOnRequest(self):
		self.sendRequest(url="/ab?cmd=valve_on")
	def sendValveOffRequest(self):
		self.sendRequest(url="/ab?cmd=valve_off")
	def sendValveFloatRequest(self):
		self.sendRequest(url="/ab?cmd=valve_float")

	def sendBkOffRequest(self):
		self.sendRequest(url="/ab?cmd=configure_bk&enabled=false")

	def sendHltOffRequest(self):
		self.sendRequest(url="/ab?cmd=configure_hlt&enabled=false")

app = Application()
app.master.title('Autobrew Python UI')
app.mainloop()
