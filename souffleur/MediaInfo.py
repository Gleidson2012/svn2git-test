import os
import pygtk
pygtk.require('2.0')
import gobject
gobject.threads_init()
import pygst
pygst.require('0.10')
import gst
from gst.extend import discoverer

class Media:
    has_audio = None
    has_video = None 
    framerate = None
    sourceURI = None
    MIME = None
    videoLengthNS = None
    videoLengthS = None
    videoCaps = None
    videoWidth = None
    videoHeight = None

class MediaInfo:

    def __init__(self, file, uri):
	self.media = Media()
	self.media.source = file
	self.media.sourceURI = uri
	self.discover(file)
	self.notDone = True

    def discover(self,path):
	d = discoverer.Discoverer(path)
	d.connect('discovered',self.cb_discover)
	d.discover()

    def cb_discover(self, d, ismedia):
	if ismedia:
	    self.media.MIME = d.mimetype
	    if d.is_video:
		self.media.has_video = True
		self.media.framerate = float(d.videorate.num)/float(d.videorate.denom)
		self.media.videoLengthNS = d.videolength
		self.media.videoLengthS = float(d.videolength)/float(gst.MSECOND)/1000.0
		self.media.videoCaps = d.videocaps
		self.media.videoHeight = d.videoheight
		self.media.videoWidth = d.videowidth
	    if d.is_audio:
		self.media.has_audio = True
	    self.notDone = False

    def poll(self):
	return self.notDone

    def getMedia(self):
	return self.media
