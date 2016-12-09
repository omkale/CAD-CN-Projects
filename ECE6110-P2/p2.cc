#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

// Network topology
//                                           n5(UDP Source)
//                                           |
//                                           |  
//                                           | 
//   (TCP Source)                            |                              (TCP Sink)
//       n0 ---------------n1----------------n2---------------n3--------------n4
//                         |                                   |
//             5 Mbps      |     1 Mbps             1 Mbps     | 5Mbps       5 Mbps 
//             10 ms       |     20 ms              20 ms      |  10 ms       10 ms
//                         |                                   |
//                         n7(TCP Source)                     n6(UDP Sink)


// - Flow from n0 to n4 using BulkSendApplication.
// - Flow from n5 to n4 using UdpClient.
// - Flow from n7 to n4 using BulkSendApplication.



using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("REDExample");



int main(int argc, char **argv)
{
    // These parameters have been chosen as specified in the Tuning Red Paper that compares RED vs Droptail queue
    uint32_t    maxBytes = 0;  //keep sending data(Since set to zero, it is unlimited)
    uint32_t    qSize = 32000;
    uint32_t    winSize = 32000;
    string      RED_Droptail = "Droptail";
    string      dataRate = "0.5Mbps";        //data rate for the UDP source
    string      RoundTTime = "5ms";       //round trip time, that is actually a variable delay introduced on links 01,17,34 
    double      minTh = 5;
    double      maxTh = 15;
    double      maxP = 50;
    double      queuewt = 1.0/128.0;         //this is the queue weight
    uint32_t    pktSize = 128;
    uint32_t    qlen = 480*128;
    string      Bottlenecklink_BW= "1Mbps";
    string      Bottlenecklink_Delay = "20ms";
    
    
    //Parameters that can be changed at command line
    CommandLine cmd;
    cmd.AddValue("qSize","queue size",qSize);
    cmd.AddValue("winSize","window size",winSize);
    cmd.AddValue("RED_Droptail","red_droptail",RED_Droptail);
    cmd.AddValue("dRate","dRate",dataRate);
    cmd.AddValue("RTT","RTT",RoundTTime);
    cmd.AddValue("minTh","minTh",minTh);
    cmd.AddValue("maxTh","maxTh",maxTh);
    cmd.AddValue("maxP","maxP",maxP);
    cmd.AddValue("queuewt","queuewt",queuewt);
    cmd.AddValue("qlen","qlen",qlen);

    cmd.Parse(argc,argv);
    
    //Parameters that are set to defalut values
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (winSize));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
    Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (maxP));
    Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (qlen));
    

    NodeContainer p2pNodes;
    p2pNodes.Create (8);       //total number of nodes in the topology considered, I have included 8 nodes

    // Setting up the desired network topology

    PointToPointHelper ptpRouter;
    PointToPointHelper ptpNode01;
    PointToPointHelper ptpNode17;
    PointToPointHelper ptpNode34;
    PointToPointHelper ptpNode52;
    PointToPointHelper ptpNode63;

    
    if ((RED_Droptail != "RED") && (RED_Droptail != "Droptail"))
    {
      NS_ABORT_MSG ("Invalid queue type: Use --RED_Droptail=RED or --RED_Droptail=DropTail");
    }

    if(RED_Droptail == "Droptail")
    {
       ptpRouter.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
       ptpRouter.SetChannelAttribute("Delay", StringValue("20ms"));
       ptpRouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(qSize));
       ptpRouter.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));
       std::cout<<"Queue Size:"<<qSize<<" Window Size:"<<winSize<<" RTT:"<<RoundTTime<<" DataRate:"<<dataRate;
    } 
    else if (RED_Droptail == "RED")
    {
       minTh *= pktSize; 
       maxTh *= pktSize;
       ptpRouter.SetDeviceAttribute  ("DataRate", StringValue (Bottlenecklink_BW));
       ptpRouter.SetChannelAttribute ("Delay", StringValue (Bottlenecklink_Delay));
       ptpRouter.SetQueue("ns3::RedQueue","QueueLimit", UintegerValue(qlen));
       ptpRouter.SetQueue("ns3::RedQueue","Mode", StringValue("QUEUE_MODE_BYTES"));
       ptpRouter.SetQueue("ns3::RedQueue","QW", DoubleValue(queuewt));
       ptpRouter.SetQueue ("ns3::RedQueue",
                               "MinTh", DoubleValue (minTh),
                               "MaxTh", DoubleValue (maxTh),
                               "LinkBandwidth", StringValue (Bottlenecklink_BW),
                               "LinkDelay", StringValue (Bottlenecklink_Delay)); 
     std::cout <<"MinTh:"<<minTh<<" MaxTh:"<<maxTh<<" MaxP:"<<maxP<<" RTT:"<<RoundTTime<<" DataRate:"<<dataRate;
    }

    ptpNode01.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptpNode01.SetChannelAttribute("Delay", StringValue(RoundTTime));

    ptpNode17.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptpNode17.SetChannelAttribute("Delay", StringValue(RoundTTime));

    ptpNode34.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptpNode34.SetChannelAttribute("Delay", StringValue(RoundTTime));

    ptpNode52.SetDeviceAttribute("DataRate", StringValue("6Mbps"));
    ptpNode52.SetChannelAttribute("Delay", StringValue("10ms"));

    ptpNode63.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptpNode63.SetChannelAttribute("Delay", StringValue("10ms"));

    //NodeContainer object is defined that holds the list of different devices on the network
    NodeContainer link1 (p2pNodes.Get(0), p2pNodes.Get(1));
    NodeContainer link17 (p2pNodes.Get(1), p2pNodes.Get(7));
    NodeContainer router1 (p2pNodes.Get(1), p2pNodes.Get(2));
    NodeContainer router2 (p2pNodes.Get(2), p2pNodes.Get(3));
    NodeContainer link4 (p2pNodes.Get(3), p2pNodes.Get(4));
    NodeContainer link52 (p2pNodes.Get(5),p2pNodes.Get(2));
    NodeContainer link63 (p2pNodes.Get(3),p2pNodes.Get(6));

    //Here we install the links between the nodes
    NetDeviceContainer NodeLink1;
    NodeLink1 = ptpNode01.Install(link1); 

    NetDeviceContainer RouterLink1;
    RouterLink1 = ptpRouter.Install(router1);

    NetDeviceContainer RouterLink2;
    RouterLink2 = ptpRouter.Install(router2);

    NetDeviceContainer NodeLink2;
    NodeLink2 = ptpNode34.Install(link4);

    NetDeviceContainer NodeLink52;
    NodeLink52 = ptpNode52.Install(link52);
 
    NetDeviceContainer NodeLink63;
    NodeLink63 = ptpNode63.Install(link63);

    NetDeviceContainer NodeLink17;
    NodeLink17 = ptpNode17.Install(link17);

    //installs the protocol stack on the nodes created
    InternetStackHelper stack;
    stack.Install(p2pNodes);
    
    //now we assign IP addresses to the interfaces
    NS_LOG_INFO ("Assign IP Addresses.");

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface1;
    p2pInterface1 = address.Assign (NodeLink1);

    Ipv4AddressHelper address1;
    address1.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface2;
    p2pInterface2 = address1.Assign (RouterLink1);

    Ipv4AddressHelper address2;
    address2.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface3;
    p2pInterface3 = address2.Assign (RouterLink2);

    Ipv4AddressHelper address3;
    address3.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface4;
    p2pInterface4 = address3.Assign (NodeLink2);

    Ipv4AddressHelper address4;
    address4.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface5;
    p2pInterface5 = address4.Assign (NodeLink52);

    Ipv4AddressHelper address5;
    address5.SetBase ("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface6;
    p2pInterface6 = address5.Assign (NodeLink63);

    Ipv4AddressHelper address6;
    address6.SetBase ("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterface7;
    p2pInterface7 = address6.Assign (NodeLink17);


    //Populating the routing tables.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();   
    
    NS_LOG_INFO ("Create applications.");

    //BulkSendApplication is installed on node 0
    uint16_t port = 68;  

    BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterface4.GetAddress (1), port));
     //Data to be sent in bytes from TCP Source at n0
    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    source.SetAttribute ("SendSize", UintegerValue (pktSize));
    ApplicationContainer sourceApps = source.Install (p2pNodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (10.0));

    BulkSendHelper source2 ("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterface4.GetAddress (1), port));
     //Data to be sent in bytes from TCP Source at n7.
    source2.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    source2.SetAttribute ("SendSize", UintegerValue (pktSize));
    ApplicationContainer sourceApps2 = source2.Install (p2pNodes.Get (7));
    sourceApps2.Start (Seconds (1.0));
    sourceApps2.Stop (Seconds (10.0));

    //Create a PacketSinkApplication and install it on node 1
    
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                            InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (p2pNodes.Get (4));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (10.0));

    
    DataRate x(dataRate);
    OnOffHelper clientHelper ("ns3::UdpSocketFactory", InetSocketAddress (p2pInterface6.GetAddress (1), port));
    clientHelper.SetConstantRate(x,pktSize);
    clientHelper.SetAttribute("PacketSize", UintegerValue (pktSize));
    ApplicationContainer srcApps = clientHelper.Install (p2pNodes.Get (5));
    srcApps.Start (Seconds (1.0));
    srcApps.Stop (Seconds (10.0));


    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer apps;
    apps.Add (packetSinkHelper.Install (p2pNodes.Get (4)));
    apps.Start (Seconds (0.0));
    apps.Stop (Seconds (10.0));

    Address sinkLocalAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", sinkLocalAddress2);
    ApplicationContainer apps2;
    apps2.Add (packetSinkHelper2.Install (p2pNodes.Get (6)));
    apps2.Start (Seconds (0.0));
    apps2.Stop (Seconds (10.0));

    Simulator::Stop (Seconds (10.0));
    Simulator::Run();
    
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));    
    std::cout << " Goodput TCP:"<< (double)sink1->GetTotalRx () / (double)10.0;

    Ptr<PacketSink> sink2 = DynamicCast<PacketSink> (apps2.Get (0));    
    std::cout << " Goodput UDP:"<< (double)sink2->GetTotalRx () / (double)10.0<< std::endl;

    Simulator::Destroy();

    return 0;
}
