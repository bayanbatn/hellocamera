#ifndef _MYAUTOFOCUS_H
#define _MYAUTOFOCUS_H

#include <FCam/Tegra/AutoFocus.h>
#include <FCam/Tegra/Lens.h>
#include <FCam/Base.h>
#include <android/log.h>

#define NUM_INTERVALS 13
#define NUM_LOOPS 2
#define RECT_EDGE_LEN 40
#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480

class MyAutoFocus : public FCam::Tegra::AutoFocus {
public:
       MyAutoFocus(FCam::Tegra::Lens *l, FCam::Rect r = FCam::Rect()) : FCam::Tegra::AutoFocus(l,r) {
    	   lens = l;
    	   rect = r;
    	   rect.width = RECT_EDGE_LEN;
    	   rect.height = RECT_EDGE_LEN;
    	   rect.x = -1;
    	   rect.y = -1;
    	   /* [CS478]
    	    * Do any initialization you need.
    	    */
    	   nearFocus = lens->nearFocus();
    	   farFocus = lens->farFocus();
    	   for (int i = 0; i < NUM_INTERVALS; i++)
    		   sharpVals[i] = 0.0f;
       }

       void startSweep() {
    	   if (!lens) return;
    	   /* [CS478]
    	    * This method should initiate your autofocus algorithm.
    	    * Before you do that, do basic checks, e.g. is the autofocus
    	    * already engaged?
    	    */
    	   //if (!afActive) return;
    	   if (focusing) return;
    	   bestFocalDist = -1.0f;

    	   sweepStart = farFocus;
    	   sweepEnd = nearFocus;
    	   dioptreInc = (nearFocus - farFocus)/(NUM_INTERVALS-1);
    	   itvlCount = 0;
    	   loopCount = 0;

    	   lens->setFocus(sweepStart);
    	   itvlCount++;

    	   focusing = true;
    	   logDump();
       }

       typedef unsigned char uchar;

       int computeImageContrast(FCam::Image &image)
       {
    	   LOG("MYFOCUS compute contrast begin\n======================\n");

    	   unsigned int sum = 0;
    	   int totalValue = 0;
    	   int highX = rect.x + rect.width - 1;
    	   int highY = rect.y + rect.height - 1;
    	   for(int x = rect.x + 1; x < highX; x++)
    	   {
    		   for(int y = rect.y + 1; y < highY; y++)
    		   {
    			   uchar* row1 = image(x-1, y-1);
    			   uchar* row2 = image(x-1, y);
    			   uchar* row3 = image(x-1, y+1);

    			   sum = row1[0] + row1[1] + row1[2] + row2[0] + row2[2] +row3[0] + row3[1] + row3[2];
    			   sum = sum >> 3;
    			   int temp = row2[1] - sum;
    			   //temp = temp < 0 ? -temp : temp;

    			   if (x == rect.x + 10 && (y >= rect.y + 5 && y <= rect.y + 15))
    				   LOG("MYFOCUS pixel contrast value: %d\n", temp);
    			   totalValue += temp * temp;
    		   }
    	   }
    	   LOG("MYFOCUS total value: %d\n", totalValue);
    	   LOG("MYFOCUS compute contrast end\n======================\n");
    	   return totalValue;
       }

       void update(const FCam::Frame &f) {
    	   /* [CS478]
    	    * This method is supposed to be called in order to inform
    	    * the autofocus engine of a new viewfinder frame.
    	    * You probably want to compute how sharp it is, and possibly
    	    * use the information to plan where to position the lens next.
    	    */
    	   /* Extract frame and do stuff */

    	   FCam::Image image = f.image();
    	   if (!image.valid()){
    		   sharpVals[itvlCount-1] = -1;
    		   LOG("MYFOCUS Invalid image\n");
    	   }

    	   LOG("MYFOCUS The expected focus setting: %f\n", (sweepStart + (itvlCount - 1) * dioptreInc));
    	   float actualFocus = (float) f["lens.focus"];
    	   LOG("MYFOCUS The average focus setting during the frame: %f\n", actualFocus);

    	   //float temp = (nearFocus - farFocus)/2 - (sweepStart + (itvlCount - 1) * dioptreInc);
    	   sharpVals[itvlCount-1] = computeImageContrast(image);

    	   if (itvlCount != NUM_INTERVALS){
    		   lens->setFocus(sweepStart + itvlCount * dioptreInc);
    		   itvlCount++;
    		   return;
    	   }
    	   loopCount++;
    	   int maxIdx = findMaxIdx();

    	   //Went through sufficient number of loops
    	   if (loopCount >= NUM_LOOPS)
    	   {
    		   bestFocalDist = sweepStart + maxIdx * dioptreInc;
    		   LOG("MYFOCUS The best focus setting: %f\n", bestFocalDist);
    		   focusing = false;
    		   lens->setFocus(bestFocalDist);
    		   return;
    	   }
    	   itvlCount = 0;
    	   sweepEnd = std::min(nearFocus, (sweepStart + (maxIdx+1) * dioptreInc));
    	   sweepStart = std::max(farFocus, (sweepStart + (maxIdx-1) * dioptreInc));
    	   dioptreInc = (sweepEnd - sweepStart) / (NUM_INTERVALS - 1);
    	   lens->setFocus(sweepStart);
    	   itvlCount++;
    	   logDump();
       }

       bool isFocusing()
       {
    	   return focusing;
       }

       int findMaxIdx(){
    	   int max = -1;
    	   int maxIdx = -1;
    	   for (int i = 0; i < NUM_INTERVALS; i++)
    	   {
    		   if (sharpVals[i] > max)
    		   {
    			   max = sharpVals[i];
    			   maxIdx = i;
    		   }
    	   }
    	   logArrayDump();
    	   LOG("MYFOCUS max idx : %d\n", maxIdx);
    	   return maxIdx;
       }

       void setRect(int x, int y, int width = RECT_EDGE_LEN, int height = RECT_EDGE_LEN)
       {
    	   rect.x = std::max(x - RECT_EDGE_LEN / 2, 0);
    	   rect.y = std::max(y - RECT_EDGE_LEN / 2, 0);
    	   rect.width = std::min(width + rect.x, IMAGE_WIDTH) - rect.x;
    	   rect.height = std::min(height + rect.y, IMAGE_HEIGHT) - rect.y;
    	   //logRectDump();
       }

       void logDump()
       {
    	   LOG("MYFOCUS LOG DUMP BEGIN\n======================\n");
    	   LOG("MYFOCUS sweep start: %f\n", sweepStart);
    	   LOG("MYFOCUS sweep end: %f\n", sweepEnd);
    	   LOG("MYFOCUS The near focus len: %f\n", nearFocus);
    	   LOG("MYFOCUS The far focus len: %f\n", farFocus);
    	   LOG("MYFOCUS Focal inc: %f\n", dioptreInc);
    	   LOG("MYFOCUS interval count: %d\n", itvlCount);
    	   LOG("MYFOCUS loop count: %d\n", loopCount);
    	   LOG("MYFOCUS Focusing?: %d\n", focusing);
    	   LOG("MYFOCUS LOG DUMP END\n======================\n");
       }

       void logArrayDump()
       {
    	   LOG("MYFOCUS LOG ARRAY DUMP BEGIN\n======================\n");
    	   for (int i = 0; i < NUM_INTERVALS; i++)
    	   {
    		   LOG("MYFOCUS array index: %d, val: %d\n", i, sharpVals[i]);
    	   }
    	   LOG("MYFOCUS LOG ARRAY DUMP END\n======================\n");
       }

       void logRectDump()
       {
    	   LOG("MYFOCUS LOG RECT DUMP BEGIN\n======================\n");
    	   LOG("MYFOCUS rect x: %d\n", rect.x);
    	   LOG("MYFOCUS rect y: %d\n", rect.y);
    	   LOG("MYFOCUS rect width: %d\n", rect.width);
    	   LOG("MYFOCUS rect height: %d\n", rect.height);
    	   LOG("MYFOCUS LOG RECT DUMP END\n======================\n");
       }


private:
       FCam::Tegra::Lens* lens;
       FCam::Rect rect;
       /* [CS478]
        * Declare any state variables you might need here.
        */
       float sweepStart;//farFocus - low range of dioptres
       float sweepEnd;
       float dioptreInc;
       int itvlCount;
       int loopCount;
       //bool afActive;
       int sharpVals[NUM_INTERVALS];
       float nearFocus;
       float farFocus;
       float bestFocalDist;
       bool focusing;
};

#endif
