# Vex V5 Playback

The playback system allows for the recording and mirroring of driver inputs, primarily for autonomous movements. Only a microSD card of modest capacity (~4-8GB recommended) is needed. On the brain, the microSD card slot is on the same side as the power plug. Make sure the card pressed all the way in the slot, with the side of the card that has small strips of bare metal at the end facing the brain screen.

![Location of the microSD card slot on the V5 brain](https://kb.vex.com/hc/article_attachments/44471808951700)

## Usage

When you run the program for driver control, the specified file at line 120 will be overwritten.
```
FILE* file = fopen("/usd/misc.txt", "w");
```
Ensure that these are saved as txt files, and only change the name of the specified file, not the extension.

When you run the program for autonomous, the sepecified file at line 73 will be read. 
```
FILE* file = fopen("/usd/misc.txt", "r");
```
The consistency of the playback system relies on synchronized delay between reading each line and applying the inputs across recording and reading. Do not change the delays unless you know what you are doing.
## FAQ

#### Is this legal?

Yes. As of version 2.2 of the Push Back handbook, autonomous is defined as such:
```
Autonomous Period - A time period during which Robots operate and react only to sensor inputs and
pre-programmed commands.
```

#### The file formatting doesn't match what I have on my robot. How can I change this?

On lines 93 and 192, the standard library functions used are fscanf and fprintf. Look at the documentation here for how to format everything to your inputs:

[https://cplusplus.com/reference/cstdio/fscanf/]

[https://cplusplus.com/reference/cstdio/fprintf/]

Just make sure that you keep the \n to create new lines or it won't be parsed correctly.

## Roadmap

- User interface during initialization to allow for easy selection, creation, overwriting, deletion, and extension of files without requiring the microSD card to be removed and modified manually or the specified files to be changed and uploaded constantly.

- Active rotation correction using a proportional controller and inertial sensor, increasing consistency despite field variation.
