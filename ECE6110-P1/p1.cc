#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

using namespace ns3;

int main(int argc, char **argv)
{
    RngSeedManager::SetSeed (11223344);
    Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
    U->SetAttribute ("Stream", IntegerValue (6110));
    U->SetAttribute ("Min", DoubleValue (0.0));
    U->SetAttribute ("Max", DoubleValue (0.1));

    uint32_t nFlows = 10;
    uint32_t queueSize = 64000;
    uint32_t windowSize = 2000;
    uint32_t segSize = 512;
    

    CommandLine cmnd;
    cmnd.AddValue("queueSize","queue size",queueSize);
    cmnd.AddValue("windowSize","window size",windowSize);
    cmnd.AddValue("segSize","segment size",segSize);
    cmnd.AddValue("nFlows","nflows",nFlows);
    cmnd.Parse(argc,argv);

    double start[nFlows];

    for(unsigned int i = 0;i < nFlows; i++)
    {
        start[i] = U->GetValue();
        //cout<<"Start time: "<<start[i]<<endl;
    }

    uint32_t maxBytes = 0;
    // Options
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
    
    //LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    //LogComponentEnable("PointToPointDumbbellHelper", LOG_LEVEL_INFO);

    std::string animFile = "p1.xml" ;  // Name of file for animation output
    // Setting up network topology
    // It is a dumbbell network with two leaves on each side
    PointToPointHelper ptprouter;
    PointToPointHelper ptpleaf;

    ptprouter.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    ptprouter.SetChannelAttribute("Delay", StringValue("20ms"));
    ptprouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
    ptprouter.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));

    ptpleaf.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptpleaf.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointDumbbellHelper dumbbell(nFlows, ptpleaf, nFlows, ptpleaf, ptprouter);
    
    InternetStackHelper stack;
    dumbbell.InstallStack(stack);

    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "255.255.255.0"),
                                 Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                                 Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));
    
   
    //Populate the routing tables.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();   
    // Setting up applications
    ApplicationContainer clientApps[nFlows];
    ApplicationContainer serverApps[nFlows];

    // Just one TCP connection for now

   
    for(unsigned int i = 0;i < nFlows; i++)
    {
     BulkSendHelper clientHelper ("ns3::TcpSocketFactory", InetSocketAddress (dumbbell.GetRightIpv4Address(i), 9));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientApps[i] = clientHelper.Install (dumbbell.GetLeft(i));
        clientApps[i].Start (Seconds (start[i]));
        clientApps[i].Stop (Seconds (10.0));
    }

    
    for(unsigned int i = 0;i < nFlows; i++)
    {
        // TCP Servers
        PacketSinkHelper serverHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(),9));
        AddressValue local(InetSocketAddress(dumbbell.GetRightIpv4Address(i), 9));
        serverHelper.SetAttribute("Local", local);
        serverApps[i].Add(serverHelper.Install(dumbbell.GetRight(i)));
        // Setting up simulation
        serverApps[i].Start(Seconds (start[i]));
        serverApps[i].Stop(Seconds(10.0));
    }
    
    //For the animation files
    dumbbell.BoundingBox (1, 1, 100, 100);
    AnimationInterface anim (animFile);
    anim.EnablePacketMetadata (); 
    anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10)); 
   
    Simulator::Stop (Seconds (10.0));        //stops the simulator
    Simulator::Run();

    for(unsigned int i = 0;i < nFlows; i++)
    {
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps[i].Get (0));    
        std::cout << "flow"<<" "<<i<<" "<<"windowSize"<<" "<<windowSize<<" "<<"queueSize"<<" "<<queueSize<<" "<<"segSize"<<" "<<segSize<<" "<<"goodput"<<" "<< (double)sink1->GetTotalRx () / (double) (10.0 - start[i])  << std::endl;
    }
    Simulator::Destroy();
    return 0;
}
