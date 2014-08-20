# Scrub 

Scrub  iteratively  writes  patterns  on  files or disk devices to make
retrieving the data more difficult.

## Modes

Scrub operates  in one  of  three modes:

1) The special file corresponding to an entire disk is scrubbed and all
data on it is destroyed.  This mode is selected if file is a  character
or block special file.  This is the most effective method.

2)  A  regular  file  is  scrubbed  and only the data in the file (and
optionally its name in the directory entry)  is destroyed.   The  file
size  is  rounded up to fill out the last file system block.  This mode
is selected if file is a regular file.  See Caveats below.

3) directory is created and filled with files until the file system  is
full,  then the files are scrubbed as in 2). This mode is selected with
the -X option.  See Caveats below.

## Methods

Scrub supports a number of standard overwrite methods.
A subset is listed below:

*nnsa* - 4-pass NNSA Policy  Letter  NAP-14.1-C  (XVI-8)  for  sanitizing
removable and non-removable hard disks, which requires overwrit-
ing all locations with a pseudorandom  pattern  twice  and  then
with a known pattern: random(x2), 0x00, verify.

*dod* - 4-pass  DoD 5220.22-M section 8-306 procedure (d) for sanitizing
removable and non-removable rigid disks which requires overwrit-
ing  all addressable locations with a character, its complement,
a random character, then verify.  NOTE: scrub performs the  ran-
dom  pass first to make verification easier: random, 0x00, 0xff,
verify.

*bsi* - 9-pass method recommended by the German Center  of  Security  in
Information  Technologies	 (http://www.bsi.bund.de): 0xff, 0xfe,
0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f.

*gutmann* - The canonical 35-pass  sequence  described  in  Gutmann’s
paper cited below.

*schneier* - 7-pass method described by Bruce Schneier in
_Applied Cryptography_ (1996): 0x00, 0xff, random(x5)

*pfitzner7* - Roy Pfitzner’s 7-random-pass method: random(x7).

*pfitzner33* - Roy Pfitzner’s 33-random-pass method: random(x33).

*usarmy* - US Army AR380-19 method: 0x00, 0xff, random.
(Note:  identical to  DoD 522.22-M section 8-306 procedure (e)
for sanitizing magnetic core memory).

## Caveats 

Scrub may be insufficient to thwart heroic efforts to recover  data  in
an  appropriately  equipped lab.  If you need this level of protection,
physical destruction is your best bet.

The effectiveness of scrubbing regular files through a file system will
be  limited  by the OS and file system.	File systems that are known to
be problematic are journaled, log structured, copy-on-write, versioned,
and network file systems.  If in doubt, scrub the raw disk device.

Scrubbing free blocks in a file system with the -X method is subject to
the same caveats as scrubbing regular files, and in addition,  is  only
useful  to the extent the file system allows you to reallocate the tar-
get blocks as data blocks in a new file.	 If in doubt,  scrub  the  raw
disk device.

On  MacOS  X  HFS  file	system,	 scrub	attempts to overwrite a file’s
resource fork if it exists.  Although MacOS X claims  it	 will  support
additional named forks in the future, scrub is only aware of the tradi-
tional data and resource forks.

scrub cannot access disk blocks that have been spared out by  the  disk
controller.   For  SATA/PATA  drives,  the ATA "security erase" command
built into the drive  controller	 can  do  this.	  Similarly,  the  ATA
"enhanced  security  erase"  can	 erase data on track edges and between
tracks.	The DOS utility HDDERASE from the  UCSD	 Center	 for  Magnetic
Recording  Research can issue these commands, as can modern versions of
Linux hdparm.  Unfortunately, the analogous SCSI	 command  is  optional
according to T-10, and not widely implemented.

## Examples

To scrub a raw device /dev/sdf1 with default NNSA patterns:

```
# scrub /dev/sdf1
scrub: using NNSA NAP-14.1-C patterns
scrub: please verify that device size below is correct!
scrub: scrubbing /dev/sdf1 1995650048 bytes (~1GB)
scrub: random  |................................................|
scrub: random  |................................................|
scrub: 0x00    |................................................|
scrub: verify  |................................................|
```

## References

[DoD 5220.22-M, _National Industrial Security Program Operating Manual_, Chapter 8, 01/1995.](http://cryptome.org/nispom/chapt8.htm)

NNSA  Policy  Letter: NAP-14.1-C, _Clearing, Sanitizing, and Destroying
Information System Storage Media, Memory	 Devices,  and	other  Related
Hardware_, 05-02-08, page XVI-8.

[_Secure	Deletion  of  Data  from  Magnetic and Solid-State Memory_, by
Peter Gutmann, Sixth USENIX Security  Symposium,	 San  Jose,  CA,  July
22-25, 1996.](https://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.html)

[_Gutmann Method_, Wikipedia](http://en.wikipedia.org/wiki/Gutmann_method)

[Darik’s boot and Nuke FAQ](http://www.dban.org/faq/)

[_Tutorial on Disk Drive Data Sanitization_, by Gordon  Hugues  and  Tom
Coughlin](http://cmrr.ucsd.edu/people/Hughes/documents/DataSanitizationTutorial.pdf)

[_Guidelines  for	 Media Sanitization_, NIST special publication 800-88,
Kissel et al, September, 2006](http://csrc.nist.gov/publications/nistpubs/800-88/NISTSP800-88_with-errata.pdf)
