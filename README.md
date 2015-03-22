# hedyattack
Codebase to the Shmoocon 2011 talk "Hop Hacking Hedy"

#Hedy Attack
===========

## Purpose
Although this started as one of my first full-fledged hardware projects, the intent was always to evaluate ways to cheaply assess deployments frequency hopping spread spectrum (FHSS) technologies (please review and I’ll assume you did). Why is this important to you? Well, because there is a rift in the radio and information techology (IT) community about how much protection FHSS provides. The most concerning of these view points is that FHSS acts as a pseudo-encryption technique. People that think FHSS is, by itself, secure believe that the channel hops happen too fast to predict and simulate without knowing the actual algorithm used to generate the hopping pattern. This manner of thinking leads to implementations that send data “protected by FHSS” in clear text or obfuscated by techniques such as XOR. In any system the transmission of clear text information could lead to information disclosure and, in the worst case, data injection.

Some deployments of FHSS are going to be more secure than others. Military units that use FHSS have the best capabilities for modifying algorithms such that hopping patterns are reduced or even eliminated. Commercial implementations are always going to be based on risk assessment. This means that the resources necessary to consistently modify algorithms to limit hopping patterns is going to suffer unless it is constantly maintained. Other limitations will also play into repeated hopping patterns. Embedded devices with limited memory and processing capabilities naturally limit the effective rotation of generated hopping patterns. Poor implementations of FHSS will reduce this even more and could result in one pattern being repeated over and over.

The intent of security researchers is to identify weak implementations in any technology. This lead our team to focus on producing tools that would provide engineers, IT administrators, and security professionals with the capabilities to assess FHSS deployments. As weak implementations are identified they can be improved with newer technologies or augmented with additional security controls.

FHSS itself is not a vulnerable or “broken” technology. Just like most technologies it must be deployed and configured correctly to provide its intended functionality. It is the deployment that may be vulnerable. FHSS itself increases resiliency (availability) and does make it harder to intercept and interact with the transmitted data. It, like other technologies, is improved and becomes even more robust when augmented by other technologies such as encryption.

## Hedy Attack
The way our team approached identifying hopping patterns is simple in theory. Certain FHSS deployments will, when given enough time, repeat the channels to which they hop. This might happen over a year, month, week, day, hour, or even minute. By monitoring FHSS transmissions it should be possible to determine the point where an FHSS deployment repeats or, in a term that might help some understand, loop. Loops are the weakness. Once loops are know the actual algorithm that generated the pattern is not necessary. A simple ordered list is all that is necessary to intercept and interact with the complete transmission.

HedyAttack approaches this problem by analysing the minimum noise on each channel. The first step is to record the average minimum received signal strength indicator (RSSI) value of each channel. Later, any RSSI value that is greater than the minimum plus a boundary value should indicate a transmission on that channel. Thus, by quickly looping through the spectrum, transmissions on any channel should be detectable. As hopping patterns are pseudo-random scans must be very fast. To increase scan speed the actions taken by the radio and controlling microcontroller must be limited to tuning the radio and recording any detected channels that jump above the minimum value.

For FHSS sources that have known configurations, this is enough to begin the analysis. But, as with every technology, different vendors have their own methods of implementations. For instance, let’s take the frequency range we mentioned before. a simple breakdown of this range would be to cut it into 500 KHs pieces which are also referred to as channels. This would give us 53 channels and make it easy to gather data. However, the channel spacing does not have to be 500 KHs. Vendors can split this up any way they choose making their implementations have more or fewer channels. This means that for systems where the channel spacing is unknown tools must analyse and determine these values so that hopping patterns are as accurate as possible.

Similar to channel spacing, the amount of data that each vendor will send during each hop is going to vary. Data analysis to determine the hopping time increments will also be helpful when analysing the hopping patterns. As time increments decrease these values will be very valuable to determine the type of equipment necessary to conduct the research.

Hedy Attack provides all of these tools that attempt to provide all of the above with the exception of determining hopping time increments. Time increments will be worked into the toolset as the other tools become more stable.

## Hardware
The hardware selected for this project was the CC1111EMK868-915 Evaluation Module Kit. This was picked because of the cost, Chipcon radio, TI support, USB capabilities, and breakout of the Data Debug pins. The fact that other pins were broken out was an added bonus. Extra added bonus is that work for the CC1111EMK will could also be leveraged for work with the CC2511EMK Evaluation Module Kit and other implementations of Chipcon chips.

Of course interaction with the CC1111EMK would not be possible without the Goodfet. Travis and his neighbours have developed tools such as goodfet.cc that allow for easy debugging and interaction with the Chipcon radio. All of the capabilities provided by the Goodfet are also provided by the CC Debugger. However, I recommend that you get a Goodfet and use it because of its flexibility. As you develop you will move to other platforms and it is very likely that the Goodfet does, or will, provide you the support you need.

## Source Code
Since the talk we have firmed up the code base and uploaded it to HedyAttack at Google Code. The primary tools (at the time of this post) are:

* specan – Spectrum analysis code ported from the IM-ME project. This code was the starting point for all of the other tools. It has had the display functionality removed. The scan data is written to XDATA and can be pulled using the Data Debug functionality provided by the Goodfet. The specantap_gnuplot.py client can be used to display the results. As the
* maxscan – Spectrum analysis code with a focus on the 902 thru 928 MHz range. This tool generates data in the same manner as specan. The best way to display this data is to use the maxscan_gnuploy.py client.
* hoptrans – Transmission code that mimics frequency hopping. This code can be adjusted to hop at different speeds and across as many channels as configured. Default is set to a channel spacing of 500 KHz which equates to 53 channels. The intent of this tool is to provide a test environment. Using this in conjunction with the other tools will require a second CC1111EMK.
* minscan – This tool starts out by recording the average minimum RSSI value for each channel using a series of initialisation passes. Once the minimum values have been recorded the tool changes mode to monitor ups in the RSSI value that should indicate transmission on that channel. To track when jumps occurred a loop counter is also maintained. The loop counter lets us create a tracked array which will contain an ordered list of channel hops. This list can be pulled off using the minscan.py client.
* hedyattack – The culmination of all the tools. This tool initialises in much the same manner as minscan. Several iterations are run to detect the minimum and maximum values of each channel. Initially this information is used to attempt to determine the channel spacing of the FHSS device. Although channel spacing can be determine programmatically reviewing the data manually is also helpful. After the initial iterations the tool moves onto detecting and recording hops in the same manner as minscan. The added strength of the hedyattack tool is the USB functionality. This means that interactions with the radio and XDATA no longer need to be accomplished using the Data Debug feature which actually pauses the microprocessor to function.
* Makefile – each tools has a corresponding Makefile in their individual tool folders. The Makefiles build the corresponding firmware and place it in the firmware directory. The Makefiles can also be used to have the Goodfet install the firmware onto the CC1111EMK.

## Get To Hacking…err…Assessing
That is it. HedyAttack in a nutshell. A methodology and toolset to be used for assessing the FHSS implementations and deployments. Hopefully it is helpful. Should you wish to join this effort just drop the administrators at HedyAttack a message. We’ll get back to you as soon as possible.

Go forth and do good things,

Don C. Weber
