# Vex V5 Playback

The playback system allows for the recording and mirroring of driver inputs, primarily for autonomous movements. Only a microSD card of modest capacity (~4-8GB recommended) is needed. On the brain, the microSD card slot is on the same side as the power plug. Make sure the card pressed all the way in the slot, with the side of the card that has small strips of bare metal at the end facing the brain screen.

![Location of the microSD card slot on the V5 brain](https://kb.vex.com/hc/article_attachments/44471808951700)

## Usage

First mount the SD card and create files (use 'touch' in the terminal on Mac) with any names that you want (under ~15 characters) and no extension.

When you run the program, the controller's screen will look something like this:

```
NO FILE SELECTED
(X)CHANGE FILE
(A)CONTINUE
```

After selecting a file, pressing A to continue will change the screen to this:

```
(X)AUTONOMOUS
(Y)OVERWRITE
(A)EXTEND
```

Autnomous will run the selected file, overwrite will record driver inputs to a file (press down to end recording), and extend will first run autonomous then record and append to the file.

For competitions, you must structure your code to select a file before you plug the controller into competition control.

```
if (!pros::competition::is_connected()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE SELECTED");
        pros::delay(50);
        master.set_text(1, 0, "(X)CHANGE FILE");
        pros::delay(50);     
        while (!pros::competition::is_connected()) {
            if (master.get_digital_new_press(DIGITAL_X)) {
                fileSelection();

                master.clear();
                pros::delay(50);
                if (playbackInfo.selectedFile.empty()) {
                    master.set_text(0, 0, "NO FILE SELECTED"); 
                } else {
                    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
                }
                pros::delay(50);
                master.set_text(1, 0, "(X)CHANGE FILE");
                pros::delay(50);
            }
            pros::delay(20);
        }
    }
```

## FAQ

#### Is this legal?

Yes. As of version 2.2 of the Push Back handbook, autonomous is defined as such:
```
Autonomous Period - A time period during which Robots operate and react only to sensor inputs
and pre-programmed commands.
```

## Roadmap

- Odometry integration
