#!/usr/bin/env python

# non-web based UI for brewery
# no graphs, either. meant to be lightweight.

import Tkinter as tk
import urllib2
import tkFont
import time, threading
import json
import ttk

bkColor="#FF3A3C"
hltColor="#4CB6FF"
mtColor="#FF9A00"
retColor="#00AA20"

class Application(tk.Frame):
	def __init__(self, master=None):
		tk.Frame.__init__(self, master)

		style = ttk.Style()
		style.theme_names()
		style.theme_use()

		self.grid()
		self.createWidgets()

		self.stopped = False
		threading.Timer(1, self.queryProbes).start()

	def queryProbes(self):

		while not self.stopped:

			try:
				data = open("/var/www/html/temp_data.json", "r")
				jsonObj = json.load(data)

				if jsonObj.has_key("28.EE9B8B040000"):
					bkTempF = jsonObj["28.EE9B8B040000"]["tempF"]
					self.bkTempLabel.configure(text="" + format(bkTempF, '.2f') + " F")
				else:
					self.bkTempLabel.configure(text=" - ")


				if jsonObj.has_key("28.3AA87D040000"):
					hltTempF = jsonObj["28.3AA87D040000"]["tempF"]
					self.hltTempLabel.configure(text="" + format(hltTempF, '.2f') + " F")
				else:
					self.hltTempLabel.configure(text=" - ")

				if jsonObj.has_key("28.A1F07C040000"):
					mtTempF = jsonObj["28.A1F07C040000"]["tempF"]
					self.mtTempLabel.configure(text="" + format(mtTempF, '.2f') + " F")
				else:
					self.mtTempLabel.configure(text=" - ")

				if jsonObj.has_key("28.42AB7D040000"):
					retTempF = jsonObj["28.42AB7D040000"]["tempF"]
					self.retTempLabel.configure(text="" + format(retTempF, '.2f') + " F")
				else:
					self.retTempLabel.configure(text=" - ")


			except OSError as err:
				print("OS error: {0}".format(err))
			except Exception as e:
				print str(e)
			except:
				print("Unexpected error")
				

			time.sleep(1)

	def createWidgets(self):

		medFont = tkFont.Font(family="DejaVuSans", size=16)
		bigFont = tkFont.Font(family="DejaVuSans", size=64, weight=tkFont.BOLD)

		# Temps
		self.tempsFrame = tk.LabelFrame(self, text="Temperatures", padx=5, pady=5, font=medFont)
		self.tempsFrame.grid(row=0, column=0)

		self.bkLabel = tk.Label(self.tempsFrame, text="BK:  ", font=bigFont, fg=bkColor)
		self.bkLabel.grid(row=0, column=0)
		self.bkTempLabel = tk.Label(self.tempsFrame, text="-", font=bigFont, fg=bkColor)
		self.bkTempLabel.grid(row=0, column=1)

		self.hltLabel = tk.Label(self.tempsFrame, text="HLT:  ", font=bigFont, fg=hltColor)
		self.hltLabel.grid(row=1, column=0)
		self.hltTempLabel = tk.Label(self.tempsFrame, text="-", font=bigFont, fg=hltColor)
		self.hltTempLabel.grid(row=1, column=1)

		self.mtLabel = tk.Label(self.tempsFrame, text="MT:  ", font=bigFont, fg=mtColor)
		self.mtLabel.grid(row=2, column=0)
		self.mtTempLabel = tk.Label(self.tempsFrame, text="-", font=bigFont, fg=mtColor)
		self.mtTempLabel.grid(row=2, column=1)

		self.retLabel = tk.Label(self.tempsFrame, text="Ret:  ", font=bigFont, fg=retColor)
		self.retLabel.grid(row=3, column=0)
		self.retTempLabel = tk.Label(self.tempsFrame, text="-", font=bigFont, fg=retColor)
		self.retTempLabel.grid(row=3, column=1)

		self.quitButton = tk.Button(self, text='Quit', command=self.quit)
		self.quitButton.grid(row=1, column=0)

app = Application()
app.master.title('Autobrew Temps Python UI')
app.mainloop()
