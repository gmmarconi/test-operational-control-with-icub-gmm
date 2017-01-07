#include <string>

#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;


/***************************************************/
class CtrlModule: public RFModule
{
protected:
    PolyDriver drvArm, drvGaze;
    ICartesianControl *iarm;
    IGazeControl      *igaze;    

    BufferedPort<ImageOf<PixelRgb> > imgLPortIn,imgRPortIn;
    Port imgLPortOut,imgRPortOut;
    RpcServer rpcPort;    
    
    Mutex mutex;    // basically a semaphor
    Vector cogL,cogR,initX,initO;
    bool okL,okR;

    /***************************************************/
    // Gets the center of gravity of the blue ball seen by the iCub
    // in pixel coordinates
    bool getCOG(ImageOf<PixelRgb> &img, Vector &cog)
    {
        int xMean=0;
        int yMean=0;
        int ct=0;

        for (int x=0; x<img.width(); x++)
        {
            for (int y=0; y<img.height(); y++)
            {
                PixelRgb &pixel=img.pixel(x,y);
                if ((pixel.b>5.0*pixel.r) && (pixel.b>5.0*pixel.g))
                {
                    xMean+=x;
                    yMean+=y;
                    ct++;
                }
            }
        }

        if (ct>0)
        {
            cog.resize(2);
            cog[0]=xMean/ct;
            cog[1]=yMean/ct;
            return true;
        }
        else
            return false;
    }

    /***************************************************/
    Vector retrieveTarget3D(const Vector &cogL, const Vector &cogR)
    {
        Vector x;
        igaze->triangulate3DPoint(cogL,cogR,x);
        return x;
    }

    /***************************************************/
    // Has the head look at the point given by x
    // This is used to counteract the torso movements indec by the rolling action
    void fixate(const Vector &x)
    {
        igaze->lookAtFixationPoint(x);
        igaze->waitMotionDone();
        igaze->setTrackingMode(true);
    }

    /***************************************************/
    // Computes the orientation needed by the right hand to
    // make the ball roll in a human-like fashion
    Vector computeHandOrientation()
    {
        Vector oy(4), ox(4);
        oy[0]=0.0; oy[1]=1.0; oy[2]=0.0; oy[3]=+M_PI;
        ox[0]=1.0; ox[1]=0.0; ox[2]=0.0; ox[3]=-M_PI/2.0;
        Matrix Ry = yarp::math::axis2dcm(oy);        // from axis/angle to rotation matrix notation
        Matrix Rx = yarp::math::axis2dcm(ox);
        Matrix R  = Rx*Ry;                            // compose the two rotations keeping the order
        return yarp::math::dcm2axis(R);
    }

    /***************************************************/
    // Puts the hand next to the ball
    void approachTargetWithHand(const Vector &x, const Vector &o)
    {
        iarm->goToPoseSync(x,o);
        iarm->waitMotionDone();
    }

    /***************************************************/
    // If the hand is correctly positioned the iCub pusshes it
    // The movement is time constrained to impress a firm
    // acceleration to the ball. Once the iCub knows what's the
    // timing of the movement, it's just told to move the hand
    // 25 cm to the left.
    void makeItRoll(const Vector &x, const Vector &o)
    {
        double ttime;
        iarm->getTrajTime(&ttime);
        iarm->setTrajTime(0.5);
        Vector y(x);
        y[1] -=0.25;
        iarm->goToPoseSync(y,o);
        iarm->waitMotionDone();
        iarm->setTrajTime(ttime);
    }

    /***************************************************/
    // This function has the iCub look down in order to
    // have the table and the ball in its field of view.
    // It look down at the vector [ x y z ] = [ -0.3  0.0  0.0 ]
    // with respect to the base reference system of the iCub.
    void look_down()
    {
        Vector staredown(3,0.0);
        staredown[0] = -0.3;
        igaze->lookAtFixationPoint(staredown);
        igaze->waitMotionDone();
    }

    /***************************************************/
    void roll(const Vector &cogL, const Vector &cogR)
    {
        yInfo("detected cogs = (%s) (%s)",
              cogL.toString(0,0).c_str(),cogR.toString(0,0).c_str());
        Vector x=retrieveTarget3D(cogL,cogR);
        yInfo("retrieved 3D point = (%s)",x.toString(3,3).c_str());
        fixate(x);
        yInfo("fixating at (%s)",x.toString(3,3).c_str());
        Vector o=computeHandOrientation();
        yInfo("computed orientation = (%s)",o.toString(3,3).c_str());
        x[1] += 0.06;
        approachTargetWithHand(x,o);
        yInfo("approached");
        makeItRoll(x,o);
        yInfo("roll!");
    }

    /***************************************************/
    void home()
    // The iCub bows then returns to position, ready to
    // take a new order
    {
        // Define bowing position
        Vector bowX(3), bowO(3);
        bowX[0] = -0.05;
        bowX[1] = -0.15;
        bowX[2] =  0.0;
        bowO = initO;
        Vector ang(3,0.0);
        // Bows moving the arm to bow position
        // Multiple positioning commands are needed to
        // avoid collision with the table.
        iarm->goToPose(bowX,bowO);
        bowX[2] = -0.05;
        Time::delay(1);
        iarm->goToPose(bowX,bowO);
        Time::delay(3.5);
        igaze->lookAtAbsAngles(ang);
        igaze->waitMotionDone();
        // Puts arm back into position
        // Multiple positioning commands are needed to
        // avoid collision with the body.
        bowX[0] -= 0.25;
        bowX[1] += 0.05;
        bowO[0]  = -0.4;
        bowO[1]  = -0.25;
        bowO[2]  = -1.0;
        bowO[3]  =  0.0;
        iarm->goToPose(bowX,bowO);
        Time::delay(2.5);
        iarm->goToPoseSync(initX,initO);
        iarm->waitMotionDone();
        igaze->lookAtAbsAngles(ang);
        igaze->waitMotionDone();
        iarm->goToPoseSync(initX,initO);
        iarm->waitMotionDone();
    }

public:
    /***************************************************/
    bool configure(ResourceFinder &rf)
    {
        Property option;
        option.put("device","gazecontrollerclient");
        option.put("remote","/iKinGazeCtrl");
        option.put("local","/client/igaze");
        drvGaze.open(option);
        igaze = NULL;
        if (!drvGaze.isValid())
            return false;
        drvGaze.view(igaze);

        option.clear();
        option.put("device","cartesiancontrollerclient");
        option.put("remote","/icubSim/cartesianController/right_arm");
        option.put("local","/cartesian_client/right_arm");
        drvArm.open(option);
        iarm = NULL;
        if (!drvArm.isValid())
        {
            drvGaze.close();
            return false;
        }
        drvArm.view(iarm);

        Vector newDof(10,1.0);
        iarm->setDOF(newDof,newDof);
        double t0=Time::now();
        while ((!iarm->getPose(initX,initO)) && (Time::now()-t0)<2.0)
        {
            Time::delay(0.1);
        }
        if ((Time::now()-t0)>=2.0)
        {
            drvGaze.close();
            drvArm.close();
            return false;
        }
        
        iarm->getPose(initX,initO);

        imgLPortIn.open("/imgL:i");
        imgRPortIn.open("/imgR:i");
        imgLPortOut.open("/imgL:o");
        imgRPortOut.open("/imgR:o");
        rpcPort.open("/service");
        //Make any input from a Port object go to the respond() method.
        attach(rpcPort);

        return true;
    }

    /***************************************************/
    bool interruptModule()
    {
        imgLPortIn.interrupt();
        imgRPortIn.interrupt();
        return true;
    }

    /***************************************************/
    bool close()
    {
        drvArm.close();
        drvGaze.close();
        imgLPortIn.close();
        imgRPortIn.close();
        imgLPortOut.close();
        imgRPortOut.close();
        rpcPort.close();
        return true;
    }

    /***************************************************/
    bool respond(const Bottle &command, Bottle &reply)
    {
        string cmd=command.get(0).asString();
        if (cmd=="help")
        {
            reply.addVocab(Vocab::encode("many"));
            reply.addString("Available commands:");
            reply.addString("- look_down");
            reply.addString("- roll");
            reply.addString("- home");
            reply.addString("- quit");
        }
        else if (cmd=="look_down")
        {
            look_down();
            reply.addString("Yep! I'm looking down now!");
        }
        else if (cmd=="roll")
        {
            mutex.lock();
            Vector cogL=this->cogL;
            Vector cogR=this->cogR;
            bool ok=okL && okR;
            mutex.unlock();
            
            if (ok)
            {
                roll(cogL,cogR);
                reply.addString("Yeah! I've made it roll like a charm!");
            }
            else
                reply.addString("I don't see any object!");
        }
        else if (cmd=="home")
        {
            home();
            reply.addString("I've got the hard work done! Going home.");
        }
        else if (cmd=="close")
        {
            close();
        }
        else
            return RFModule::respond(command,reply);

        return true;
    }

    /***************************************************/
    double getPeriod()
    {
        return 0.0;     // sync upon incoming images
    }

    /***************************************************/
    bool updateModule()
    {
        // get fresh images
        ImageOf<PixelRgb> *imgL=imgLPortIn.read();
        ImageOf<PixelRgb> *imgR=imgRPortIn.read();

        // interrupt sequence detected
        if ((imgL==NULL) || (imgR==NULL))
            return false;

        // compute the center-of-mass of pixels of our color
        mutex.lock();
        okL=getCOG(*imgL,cogL);
        okR=getCOG(*imgR,cogR);
        mutex.unlock();

        PixelRgb color;
        color.r=255; color.g=0; color.b=0;

        if (okL)
            draw::addCircle(*imgL,color,(int)cogL[0],(int)cogL[1],5);

        if (okR)
            draw::addCircle(*imgR,color,(int)cogR[0],(int)cogR[1],5);

        imgLPortOut.write(*imgL);
        imgRPortOut.write(*imgR);

        return true;
    }
};


/***************************************************/
int main()
{
    Network yarp;
    if (!yarp.checkNetwork())
        return 1;

    CtrlModule mod;
    ResourceFinder rf;
    return mod.runModule(rf);
}

