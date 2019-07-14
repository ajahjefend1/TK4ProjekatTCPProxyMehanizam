/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 LIPTEL.ieee.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Miralem Mehic <miralem.mehic@ieee.org>
 */


// Network topology
//
//       n0 --- n1 --- n2
//        |-----tcp-----|
//
// 



#include <fstream>
#include "ns3/core-module.h" 
#include "ns3/network-module.h"
#include "ns3/qkd-helper.h" 
#include "ns3/qkd-app-charging-helper.h"
#include "ns3/qkd-graph-manager.h" 
#include "ns3/applications-module.h"
#include "ns3/internet-module.h" 
#include "ns3/flow-monitor-module.h" 
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/gnuplot.h" 
#include "ns3/qkd-send.h"
#include "ns3/error-model.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"

#include "ns3/aodvq-module.h" 
#include "ns3/dsdvq-module.h"  

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("QKD_CHANNEL_TEST");


//Create variables to test the results 
uint32_t m_bytes_sent = 0; 
uint32_t m_bytes_received = 0; 

uint32_t m_packets_sent = 0; 
uint32_t m_packets_received = 0; 

//Create help variable m_time
double m_time = 0;

//Create c++ map for measuring delay time
std::map<uint32_t, double> m_delayTable;
 
/**
   * \brief Each time the packet is sent, this function is called using TracedCallback and information about packet is stored in m_delayTable
   * \param context - used to display full path of the information provided (useful for taking information about the nodes or applications)
   * \param packet - Ptr reference to the packet itself
   */
void
SentPacket(std::string context, Ptr<const Packet> p){
    
    /* 
    //HELP LINES USED FOR TESTING
    std::cout << "\n ..................SentPacket....." << p->GetUid() << "..." <<  p->GetSize() << ".......  \n";
    p->Print(std::cout);                
    std::cout << "\n ............................................  \n";  
    */

    //Sum bytes of the packet that was sent
    m_bytes_sent  += p->GetSize(); 
    m_packets_sent++;

    //Insert in the delay table details about the packet that was sent
    m_delayTable.insert (std::make_pair (p->GetUid(), (double)Simulator::Now().GetSeconds()));
}

void
ReceivedPacket(std::string context, Ptr<const Packet> p, const Address& addr){
    
    /*
    //HELP LINES USED FOR TESTING
    std::cout << "\n ..................ReceivedPacket....." << p->GetUid() << "..." <<  p->GetSize() << ".......  \n";
    p->Print(std::cout);                
    std::cout << "\n ............................................  \n";  
    */

    //Find the record in m_delayTable based on packetID
    std::map<uint32_t, double >::iterator i = m_delayTable.find ( p->GetUid() );
    
    //Get the current time in the temp variable
    double temp = (double)Simulator::Now().GetSeconds();  
    //Display the delay for the packet in the form of "packetID delay" where delay is calculated as the current time - time when the packet was sent
    std::cout << p->GetUid() << "\t" << temp - i->second << "\n";

    //Remove the entry from the delayTable to clear the RAM memroy and obey memory leakage
    if(i != m_delayTable.end()){
        m_delayTable.erase(i);
    }

    //Sum bytes and number of packets that were sent
    m_bytes_received += p->GetSize(); 
    m_packets_received++;
}

void
Ratio(){

    std::cout << "Sent (bytes):\t" <<  m_bytes_sent
    << "\tReceived (bytes):\t" << m_bytes_received 
    << "\nSent (Packets):\t" <<  m_packets_sent
    << "\tReceived (Packets):\t" << m_packets_received 
    
    << "\nRatio (bytes):\t" << (float)m_bytes_received/(float)m_bytes_sent
    << "\tRatio (packets):\t" << (float)m_packets_received/(float)m_packets_sent << "\n";
}

//static void
//CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
//{
 // NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
//}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}


int main (int argc, char *argv[])
{

    Packet::EnablePrinting(); 
    PacketMetadata::Enable ();
    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO ("Create nodes.");
    NodeContainer n;
    n.Create (3); 

    NodeContainer n0n1 = NodeContainer (n.Get(0), n.Get (1));
    NodeContainer n1n2 = NodeContainer (n.Get(1), n.Get (2));
  
    //Underlay network - set routing protocol (if any)

    //Enable OLSR
    //OlsrHelper routingProtocol;
    AodvHelper routingProtocol; 

    InternetStackHelper internet;
    internet.SetRoutingHelper (routingProtocol);
    internet.Install (n);

    // Set Mobility for all nodes
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
    positionAlloc ->Add(Vector(0, 200, 0)); // node0 
    positionAlloc ->Add(Vector(200, 200, 0)); // node1
    positionAlloc ->Add(Vector(400, 200, 0)); // node2 

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(n);
       
    // We create the channels first without any IP addressing information
    NS_LOG_INFO ("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("100ps")); 

    NetDeviceContainer d0d1 = p2p.Install (n0n1); 
    NetDeviceContainer d1d2 = p2p.Install (n1n2);

//////// ERROR MODEL///////////

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.0001));
  d0d1.Get(1) ->SetAttribute ("ReceiveErrorModel", PointerValue (em));

///////////////////////////////

    //
    // We've got the "hardware" in place.  Now we need to add IP addresses.
    // 
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

    // Create router nodes, initialize routing database and set up the routing
    // tables in the nodes.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
 
  ///////////////////////////////////////
  // NS_LOG_INFO ("Create Applications.");

    /* Create user's traffic between v0 and v1 */
    /* Create sink app */
   uint16_t sinkPort = 8080;
    QKDSinkAppHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort)); 
    ApplicationContainer sinkApps = packetSinkHelper.Install (n.Get (2));
   sinkApps.Start (Seconds (25.));
    sinkApps.Stop (Seconds (170.));

    /* Create source app  */
    Address sinkAddress (InetSocketAddress (i1i2.GetAddress(1,0), sinkPort));
    Address sourceAddress (InetSocketAddress (i0i1.GetAddress(0,0), sinkPort));
   Ptr<Socket> overlaySocket = Socket::CreateSocket (n.Get (0),TcpSocketFactory::GetTypeId ());  
 
   Ptr<QKDSend> app = CreateObject<QKDSend> ();
    app->Setup (overlaySocket, sourceAddress, sinkAddress, 200, 0, DataRate ("5kbps"));
    n.Get (0)->AddApplication (app);
    app->SetStartTime (Seconds (25.));
    app->SetStopTime (Seconds (170.)); 
 
  ////////// PROZOR ZAGUSENJA////////
  //Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",MakeCallback(&CwndChange));
  //////////////////////////////////////////////////
Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",MakeCallback(&CwndChange));

    //////////////////////////////////////
    ////         STATISTICS
    //////////////////////////////////////

    //if we need we can create pcap files
    p2p.EnablePcapAll ("QKD_vnet_test"); 
   

///ispisivanje primljenih i poslanih paketa

  //Config::Connect("/NodeList/*/ApplicationList/*/$ns3::QKDSend/Tx", MakeCallback(&SentPacket));
  // Config::Connect("/NodeList/*/ApplicationList/*/$ns3::QKDSink/Rx", MakeCallback(&ReceivedPacket));

 
    Simulator::Stop ( Seconds (100) );
    Simulator::Run ();

    Ratio( );

    //Finally print the graphs
    Simulator::Destroy ();
}
