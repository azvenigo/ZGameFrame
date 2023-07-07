# ZImageViewer

# Usage

## Viewing
The main feature is being able to go through images as quickly as possible. After loading/displaying the initial image the viewer will pre-load images in the same folder ahead and behind so that changing images should be instantaneous.
It is configured to use up to 2GiB of memory or up to 10 pre-loaded images ahead and behind of the current image, whichever is lower.

To quit, close the window or hit 'ESC'


### Zoom/scroll
Zooming occurs around the mouse cursor. So if you want to zoom into one section of the image, wheel or right-click to examine that portion, then right-click to go back to fit or 100%.

Zoom: ALT-Mouse wheel
Toggle Fit to window/100%: RIGHT-CLICK
Scroll: click->drag


## Bringing up UI
Toggle UI:   TAB
Toggle Windowed/Fullscreen: 'F'

The control panel at the top can be viewed when the UI is visible or when moving the mouse cursor to the top of the window.

Top left shows current folder and current image.

## Changing images in current folder
Next Image:  Mouse wheel or RIGHT arrow
Prev Image:  Mouse wheel or LEFT arrow
First Image: HOME
Last Image:  END


## Managing images
A few controls are available for quickly managing.
Images can be marked for deletion later using 'DEL'. The window shows which images are marked and how many are marked total. 
When quitting there will be a confirmation dialog showing all marked images and asking whether to go back, confirm, or quit without deleting anything.
The confirmation dialog's list can be clicked on to view the marked images directly.

There are a few options for quickly moving images to other folders, including favorites, to_be_deleted, or custom set folder.
When viewing an image, hitting 'M' will bring up a dialog to select the destination folder. Once the folder is selected, every 'M' after that for that app session will instantly move the image to that folder.


Mark an image for later deletion: DEL
Move an image into a "FAV" subfolder: '1'
Move an image into a "to_be_deleted" subfolder: 'D'
Move an image to a custom folder: 'M'
 
