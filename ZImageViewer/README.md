# ZImageViewer
Photographer's aide to streamline sorting through photos after a shoot. I've found that off-the-shelf products like Adobe Lightroom or even FastStone are just not fast enough for my use.
I sometimes want to smooth zoom, but often I need to instantly look at areas of the image at 100% then back to full image to judge the quality.
I need to be able to open/close images instantly and flip through (even very large images) instantly. I also need to be able to flag my favorites or ones to delete quickly.

My ideal workflow after returning with 500-1k photos is
1) First pass is to go through and mark for deletion, photos problems with focus, composition, exposure, awkward expressions, etc. (This removes roughly 50%.)
2) Second pass is to go through and pick images with potential. Typically when you have several similar images, I pick the best. (This brings me down to 40-50 of my favorites.)
3) Run my ranking "face off" where two images are chosen at random for a snap judgement about which is best. (This uses a chess-like ELO system for ranking.)

That gets me down to 10-20 of the best images worthy of touchup and publishing.

## Supported formats
Uses the excellent stb_image library: https://github.com/nothings/stb/tree/master
JPEG, PNG, GIF, BMP, TGA, PSD (composited view only)



## How to use
ZImageViewer is folder based. The main feature is being able to go through images as quickly as possible. After loading/displaying the initial image the viewer will pre-load images in the same folder ahead and behind so that changing images should be instantaneous.
It is configured to use up to 4GiB of memory or up to 10 pre-loaded images ahead and behind of the current image, whichever is lower.

To quit, close the window or hit 'ESC'




### Zoom/scroll
Zooming occurs around the mouse cursor. So if you want to zoom into one section of the image, wheel or right-click to examine that portion, then right-click to go back to fit or 100%.

Zoom: ALT-Mouse wheel
Toggle "Fit to window" <-> 100: RIGHT-CLICK
Scroll: click->drag


## Toggling UI
Toggle UI:   TAB
Toggle Windowed/Fullscreen: 'CTRL-F'

The control panel at the top can be viewed when the UI is visible or when moving the mouse cursor to the top of the window.
Top left shows current folder and current image.
Images that have geo-location metadata (visible in the lower right) will open Google Maps in a browser if you click on the location data to show where the photo was taken.



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

## Ranking images
Images that have been rated through the ranking view will have persisted metadata. The image strip on the right is thumbnails of the top 20. Click on the thumbnail to view.
