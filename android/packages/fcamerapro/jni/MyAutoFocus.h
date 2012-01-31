#ifndef _MYAUTOFOCUS_H
#define _MYAUTOFOCUS_H

#include <FCam/Tegra/AutoFocus.h>
#include <FCam/Tegra/Lens.h>
#include <FCam/Base.h>
#include <android/log.h>

#define NUM_INTERVALS 7
#define NUM_LOOPS 2
#define RECT_EDGE_LEN 20

class MyAutoFocus : public FCam::Tegra::AutoFocus {
public:
       MyAutoFocus(FCam::Tegra::Lens *l, FCam::Rect r = FCam::Rect()) : FCam::Tegra::AutoFocus(l,r) {
    	   lens = l;
    	   rect = r;
    	   rect.width = RECT_EDGE_LEN;
    	   rect.height = RECT_EDGE_LEN;
    	   /* [CS478]
    	    * Do any initialization you need.
    	    */
    	   sharpVals = new float[NUM_INTERVALS];
    	   nearFocus = lens->nearFocus();
    	   farFocus = lens->farFocus();
       }

       void startSweep() {
    	   if (!lens) return;
    	   /* [CS478]
    	    * This method should initiate your autofocus algorithm.
    	    * Before you do that, do basic checks, e.g. is the autofocus
    	    * already engaged?
    	    */
    	   if (!afActive) return;
    	   bestFocalDist = -1;

    	   sweepStart = nearFocus;
    	   sweepEnd = farFocus;
    	   focalInc = (farFocus - nearFocus)/(NUM_INTERVALS-1);
    	   itvlCount = 0;
    	   loopCount = 0;

    	   lens->setFocus(sweepStart);
    	   itvlCount++;

    	   LOG("The near focus len: %f\n", nearFocus);
    	   LOG("The far focus len: %f\n", farFocus);
       }

       void update(const FCam::Frame &f) {
    	   /* [CS478]
    	    * This method is supposed to be called in order to inform
    	    * the autofocus engine of a new viewfinder frame.
    	    * You probably want to compute how sharp it is, and possibly
    	    * use the information to plan where to position the lens next.
    	    */
    	   /* Extract frame and do stuff */
    	   LOG("The average focus setting during the frame: %f\n", f["lens.focus"]);
    	   LOG("The index into the array: %d\n", itvlCount);
    	   LOG("The expected focus setting: %f\n", (sweepStart + itvlCount * focalInc));

    	   FCam::Image image = f.image();
    	   if (!image.valid()){
    		   sharpVals[itvlCount-1] = -1;
    		   LOG("Invalid image\n");
    	   }
    	   sharpVals[itvlCount-1] = ((farFocus - nearFocus)/2 - (sweepStart + (itvlCount - 1) * focalInc))^2;

    	   if (itvlCount != NUM_INTERVALS){
    		   lens->setFocus(sweepStart + itvlCount * focalInc);
    		   itvlCount++;
    		   return;
    	   }
    	   loopCount++;
    	   if (loopCount = NUM_LOOPS)
    	   {
    		   bestFocalDist = sweepStart + maxIdx * focalInc;
    		   LOG("The best focus setting: %f\n", bestFocalDist);
    		   return;
    	   }
    	   itvlCount = 0;
    	   int maxIdx = findMaxIdx(sharpVals);
    	   sweepStart = max(nearFocus, (sweepStart + (maxIdx-1) * focalInc));
    	   sweepEnd = min(farFocus, (sweepStart + (maxIdx+1) * focalInc));
    	   focalInc = (sweepEnd - sweepStart) / (NUM_INTERVALS - 1);

       }

       void getFocalDist()
       {
    	   return bestFocalDist;
       }

       void setAFActive(bool isActive)
       {
    	   afActive = isActive;
       }

       int findMaxIdx(float sharpVals[]){
    	   float max = -1;
    	   int maxIdx = -1;
    	   for (int i = 0; i < NUM_INTERVALS; i++)
    	   {
    		   if (sharpVals[i] > max)
    		   {
    			   max = sharpVals[i];
    			   maxIdx = i;
    		   }
    	   }
    	   return maxIdx;
       }

       void setRect(int x, int y)
       {
    	   rect.x = x;
    	   rect.y = y;
       }


private:
       FCam::Tegra::Lens* lens;
       FCam::Rect rect;
       /* [CS478]
        * Declare any state variables you might need here.
        */
       float sweepStart;
       float sweepEnd;
       float focalInc;
       int itvlCount;
       int loopCount;
       bool afActive;
       float sharpVals[];
       float nearFocus;
       float farFocus;
       float bestFocalDist;
};

#endif
