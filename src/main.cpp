#include <string>

#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>

#include <iCub/ctrl/math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;


/***************************************************/
class CtrlModule: public RFModule
{
protected:
    PolyDriver drvArm, drvGaze;
    ICartesianControl *iarm;
    IGazeControl      *igaze;
    Property           option;

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
        //mutex.lock();
        igaze->triangulate3DPoint(cogL,cogR,x);
        //mutex.unlock();
        return x;
    }

    /***************************************************/
    void fixate(const Vector &x)
    {
        igaze->lookAtFixationPoint(x);
        igaze->waitMotionDone();
    }

    /***************************************************/
    Vector computeHandOrientation()
    {
        Vector oy(4), ox(4);
        oy[0]=0.0; oy[1]=1.0; oy[2]=0.0; oy[3]=+M_PI;
        ox[0]=1.0; ox[1]=0.0; ox[2]=0.0; ox[3]=-M_PI/2.0;
        Matrix Ry = yarp::math::axis2dcm(oy);        // from axis/angle to rotation matrix notation
        Matrix Rx = yarp::math::axis2dcm(ox);
        Matrix R  = Rx*Ry;                            // compose the two rotations keeping the order
        Vector o  = yarp::math::dcm2axis(R);
        return o;
    }

    /***************************************************/
    void approachTargetWithHand(const Vector &x, const Vector &o)
    {
        iarm->goToPoseSync(x,o);
        iarm->waitMotionDone();
    }

    /***************************************************/
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

        yDebug() << "Bleep3\n";
    }

    /***************************************************/
    void look_down()
    {
        Vector staredown(3);
        staredown[0] = -0.3;
        staredown[1] =  0;
        staredown[2] =  0;
        igaze->lookAtFixationPoint(staredown);
        igaze->waitMotionDone();
    }

    /***************************************************/
    void roll(const Vector &cogL, const Vector &cogR)
    {
        mutex.lock();
        yInfo("detected cogs = (%s) (%s)",
              cogL.toString(0,0).c_str(),cogR.toString(0,0).c_str());
        Vector x=retrieveTarget3D(cogL,cogR);
        mutex.unlock();
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
    {
        iarm->goToPoseSync(initX,initO);
        iarm->waitMotionDone();
        Vector ang(3);
        ang = 0;
        igaze->lookAtAbsAngles(ang);
        igaze->waitMotionDone();
    }

public:
    /***************************************************/
    bool configure(ResourceFinder &rf)
    {
        option.clear();
        option.put("device","gazecontrollerclient");
        option.put("remote","/iKinGazeCtrl");
        option.put("local","/client/igaze");
        drvGaze.open(option);
        igaze = NULL;
        if (drvGaze.isValid())   drvGaze.view(igaze);

        option.clear();
        option.put("device","cartesiancontrollerclient");
        option.put("remote","/icubSim/cartesianController/right_arm");
        option.put("local","/cartesian_client/right_arm");
        drvArm.open(option);
        iarm    = NULL;
        if (drvArm.isValid())
        {
            drvArm.view(iarm);
        }

        Vector curDof;
        iarm->getDOF(curDof);
        Vector newDof(3);
        newDof[0]=1;    // torso pitch: 1 => enable
        newDof[1]=1;    // torso roll:  2 => skip
        newDof[2]=1;    // torso yaw:   1 => enable
        iarm->setDOF(newDof,curDof);
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
            reply.addString("- make_it_roll");
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
            if (okL && okR)
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

