#ifndef _MYAUTOFOCUS_H
#define _MYAUTOFOCUS_H

#include <FCam/Tegra/AutoFocus.h>
#include <FCam/Tegra/Lens.h>
#include <FCam/Base.h>
#include <android/log.h>

#define NUM_INTERVALS 7
#define NUM_LOOPS 2
#define RECT_EDGE_LEN 20
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

       float computeContract(FCam::Image &image)
       {
    	   return -1;//computeContrast(iamge)
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

    	   float temp = (nearFocus - farFocus)/2 - (sweepStart + (itvlCount - 1) * dioptreInc);
    	   sharpVals[itvlCount-1] = -temp * temp + 50;

    	   if (itvlCount != NUM_INTERVALS){
    		   lens->setFocus(sweepStart + itvlCount * dioptreInc);
    		   itvlCount++;
    		   return;
    	   }
    	   loopCount++;
    	   int maxIdx = findMaxIdx(sharpVals);
    	   if (loopCount >= NUM_LOOPS)
    	   {
    		   bestFocalDist = sweepStart + maxIdx * dioptreInc;
    		   lens->setFocus(bestFocalDist);
    		   LOG("The best focus setting: %f\n", bestFocalDist);
    		   focusing = false;
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

       int findMaxIdx(float sharpVals[]){
    	   float max = -1.0f;
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
    	   logRectDump();
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
    		   LOG("MYFOCUS array index: %d, val: %f\n", i, sharpVals[i]);
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
       float sharpVals[NUM_INTERVALS];
       float nearFocus;
       float farFocus;
       float bestFocalDist;
       bool focusing;
};

#endif
