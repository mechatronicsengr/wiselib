/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

/*
* File: DPS_radio.h
* Class(es): 
* Author: Daniel Gehberger - GSoC 2013 - DPS project
*/

#ifndef __RADIO_DPS_RADIO_H__
#define __RADIO_DPS_RADIO_H__

#include "config_testing.h"

#include "util/delegates/delegate.hpp"
#include "util/pstl/vector_static.h"
#include "util/pstl/map_static_vector.h"
#include "util/pstl/pair.h"

#include "radio/DPS/DPS_config.h"
#include "radio/DPS/DPS_packet.h"



namespace wiselib
{
	/**
	* \brief 
	*/
	template<typename Radio_P>
	class Connection_t
	{
		typedef Radio_P Radio;
		typedef typename Radio::node_id_t radio_node_id_t;
		
	public:
		Connection_t()
		: Pid(0),
		partner_MAC( Radio::NULL_NODE_ID ),
		connection_status(UNUSED),
		client_counter(0),
		server_counter(0),
		elapsed_time(0)
		{}
		
		enum connection_status
		{
			//Client
			UNUSED = 0,
			SENDING_DISCOVERY = 1,
			CONNECT_SENT = 2,
			
			//Server
			ADVERTISE_SENT = 4,
			ALLOW_SENT = 5,
			
			CONNECTED = 6
		};
		
		
		/**
		 * Protocol ID pair
		 */
		uint8_t Pid;
		
		/**
		 * MAC address of the communication partner (server/client)
		 */
		radio_node_id_t partner_MAC;
		
		/**
		 * Status of the DPS connection
		 */
		uint8_t connection_status;
		
		/**
		 * CNT the Client
		 */
		uint32_t client_counter;
		
		/**
		 * CNT of the server
		 */
		uint32_t server_counter;
		
		/**
		 * Elapsed time since the last activity
		 */
		uint16_t elapsed_time;
		
		/**
		 * Nonce for the connection, generated by the client in the CONNECT message
		 */
		uint32_t connection_nonce;
		
		/**
		 * Connection keys (nonce XOR negotiated key)
		 */
		uint8_t key[16];
// 		uint8_t my_connection_key[16];
// 		uint8_t partner_connection_key[16];
		
	};
	
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
	
	
	/**
	* \brief 
	*/
	template<typename Radio_P,
		typename Pair_P>
	class Protocol_t
	{
		typedef Radio_P Radio;
		typedef Pair_P node_id_t;
		typedef typename Radio::block_data_t block_data_t;
		
	public:
		Protocol_t()
		: server( false ),
		rpc_handler_delegate(RPC_handler_delegate_t()),
		buffer_handler_delegate(buffer_handler_delegate_t())
		{}
		
		/**
		 * Server / Client flag
		 */
		bool server;
		
		/**
		 * RPC_handler delegate
		 * Source, Fid, length, buffer
		 */
		typedef delegate3<int, node_id_t, uint16_t, block_data_t*> RPC_handler_delegate_t;
		RPC_handler_delegate_t rpc_handler_delegate;
		
		/**
		 * buffer handler delegate
		 * function for 2 use cases
		 * 	- get a buffer for incoming messages
		 * 	- when ACK is required then free up the buffer
		 */
		typedef delegate3<block_data_t*, block_data_t*, uint16_t, bool> buffer_handler_delegate_t;
		buffer_handler_delegate_t buffer_handler_delegate;
	};
	
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
	
	
	/**
	* \brief 
	*/
	class DPS_node_id_type
	{
	public:
		DPS_node_id_type()
		: Pid( 0 ),
		Fid( 0 ),
		ack_required( 0 )
		{}
		
		uint8_t Pid;
		uint8_t Fid;
		uint8_t ack_required;
	};
	
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
		
		
	/**
	* \brief DPS radio
	*/
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	class DPS_Radio
	{
	public:
		typedef OsModel_P OsModel;
		typedef Radio_P Radio;
		typedef Debug_P Debug;
		typedef Timer_P Timer;
		typedef Rand_P Rand;
		
		typedef DPS_Radio<OsModel, Radio, Debug, Timer, Rand> self_type;
		typedef self_type* self_pointer_t;
		
		typedef typename Radio::node_id_t radio_node_id_t;
		typedef typename Radio::size_t size_t;
		typedef typename Radio::block_data_t block_data_t;
		typedef typename Radio::message_id_t message_id_t;
		
		//node_id_t as a pair: P_ID and F_ID
		typedef DPS_node_id_type node_id_t;
		
		typedef DPS_Packet<OsModel, Radio, Debug> DPS_Packet_t;
		
		typedef Protocol_t<Radio, node_id_t> protocol_type;
		typedef wiselib::pair<uint8_t, protocol_type> newprotocol_t;
		typedef MapStaticVector<OsModel, uint8_t, protocol_type, DPS_MAX_PROTOCOLS> Protocol_list_t;
		
		typedef Connection_t<Radio> connection_type;
		typedef vector_static<OsModel, connection_type, DPS_MAX_CONNECTIONS> Connection_list_t;
		typedef typename Connection_list_t::iterator Connection_list_iterator;
		
		//TODO
		enum Pid_values
		{
			TEST_PID = 10
		};

		// --------------------------------------------------------------------
		enum ErrorCodes
		{
			SUCCESS = OsModel::SUCCESS,
			ERR_UNSPEC = OsModel::ERR_UNSPEC,
			ERR_NOTIMPL = OsModel::ERR_NOTIMPL,
			ERR_HOSTUNREACH = OsModel::ERR_HOSTUNREACH,
			NO_CONNECTION = 100
		};
		// --------------------------------------------------------------------
		
		enum SpecialNodeIds {
		BROADCAST_ADDRESS = Radio_P::BROADCAST_ADDRESS, ///< All nodes in communication range
		NULL_NODE_ID      = Radio_P::NULL_NODE_ID      ///< Unknown/No node id
		};
		
		// --------------------------------------------------------------------
// 		//TODO reduce the size?
// 		enum Restrictions {
// 			MAX_MESSAGE_LENGTH = Radio::MAX_MESSAGE_LENGTH  ///< Maximal number of bytes in payload
// 		};
		// --------------------------------------------------------------------
		///@name Construction / Destruction
		///@{
		DPS_Radio();
		~DPS_Radio();
		///@}
		
		int init( Radio& radio, Debug& debug, Timer& timer, Rand& rand )
		{
			radio_ = &radio;
			debug_ = &debug;
			timer_ = &timer;
			rand_ = &rand;
			
// 			rand_->srand( id() );
			
			return SUCCESS;
		}
		
		inline int init();
		inline int destruct();
		
		///@name Routing Control
		///@{
		int enable_radio( void );
		int disable_radio( void );
		///@}
		
		///@name Radio Concept
		///@{
		/**
		*/
		int send( node_id_t receiver, uint16_t length, block_data_t *data );
		
		/**
		 * \brief
		*/
		void receive( radio_node_id_t from, size_t length, block_data_t *data );
		
		/**
		 * \brief
		*/
		radio_node_id_t id()
		{
			return radio().id();
		}
		///@}
		
		
		
		/**
		 * \brief
		 */
		template<class T, int (T::*TMethod)(node_id_t, uint16_t, block_data_t*), block_data_t* (T::*TMethod2)(block_data_t*, uint16_t, bool)>
		int reg_recv_callback( T *obj_pnt, uint8_t Pid, bool server )
		{
			//The Pid has been already registered
			if( protocol_list_.contains( Pid ) )
				return ERR_UNSPEC;
			
			//Save the registration: function pointer and server flag
			newprotocol_t newprotocol;
			newprotocol.first = Pid;
			newprotocol.second.rpc_handler_delegate = protocol_type::RPC_handler_delegate_t::template from_method<T, TMethod>( obj_pnt );
			newprotocol.second.buffer_handler_delegate = protocol_type::buffer_handler_delegate_t::template from_method<T, TMethod2>( obj_pnt );
			newprotocol.second.server = server;
			
			protocol_list_.push_back(newprotocol);
			
			#ifdef DPS_RADIO_DEBUG
			debug().debug( "DPS: RPC handler registration for %i at %llx as %x", Pid, (long long unsigned)(radio().id()), server);
			#endif
			
			//Start the DISCOVERY if it is a client
			if( !server )
			{
				connection_type connection;
				connection.Pid = Pid;
				connection.connection_status = connection_type::SENDING_DISCOVERY;
				connection.client_counter = rand()() % (0xFFFFFFFF);
				
				Connection_list_iterator it = connection_list_.insert( connection );
				
				if( it == connection_list_.end() )
				{
					#ifdef DPS_RADIO_DEBUG
					debug().debug("DPS: Error, connection list is full");
					#endif
					return ERR_UNSPEC;
				}
				
				timer().template set_timer<self_type, &self_type::send_DISCOVERY>( DPS_TIMER_DISCOVERY_FREQUENCY, this, (void*)(it-connection_list_.begin()) );
			}
			
			return SUCCESS;
		}
		
		/**
		 * \brief
		 */
		int unreg_recv_callback( uint8_t Pid )
		{
// 			protocol_list_[Pid].rpc_handler_delegate = protocol_type::RPC_handler_delegate_t();
			return SUCCESS;
		}
		
	private:
		
		/**
		* \brief
		*/
		int send_connection_message( radio_node_id_t destination, uint8_t type, Connection_list_iterator connection );
		
		/**
		* \brief Function called by the timer in order to send DISCOVERY message
		*/
		void send_DISCOVERY( void* iterator );
		
		/**
		* \brief Function called by the timer in order to mentain timing in DPS
		*/
		void general_periodic_timer( void* );
		
		/**
		 * \brief Calculate the connection key
		 * NOTE: The nonce must be set for the connection when this function is called.
		 */
		void generate_connection_key( Connection_list_iterator connection );
		
#if DPS_FOOTER > 0
		/**
		 * \brief Calculate the footer checksum for a buffer
		 */
		void calculate_checksum_for_buffer( size_t length, block_data_t* buffer, Connection_list_iterator connection, block_data_t* MAC, bool use_request_key );
#endif
		
		//NOTE: only for TEST
		void turn_off( void* )
		{
			//disable_radio(); is not working for shawn...
			disabled_ = true;
		}
		
		bool disabled_;
		
		/**
		 * Map for the registered protocols
		 */
		Protocol_list_t protocol_list_;
		
		/**
		 * List for the connections
		 */
		Connection_list_t connection_list_;
		
		/**
		 * Saved buffer pointer from the RPC handler class
		 * Used for fragmentation & reassembling
		 */
		block_data_t* app_buffer;
		
		
		//Stored callback_id for radio
		uint8_t callback_id_;
		
		Radio& radio()
		{ return *radio_; }
		
		Debug& debug()
		{ return *debug_; }
		
		Timer& timer()
		{ return *timer_; }
		
		Rand& rand()
		{ return *rand_; }
		
		typename Radio::self_pointer_t radio_;
		typename Debug::self_pointer_t debug_;
		typename Timer::self_pointer_t timer_;
		typename Rand::self_pointer_t rand_;
		
	};
	
	/**
	* Pre-shared DISCOVERY & ADVERTISE key
	*/
	const uint8_t DPS_REQUEST_KEY[] ={112,86,44,43,207,145,21,13,37,123,182,70,194,174,152,239};
	
	// -----------------------------------------------------------------------
	// -----------------------------------------------------------------------
	// -----------------------------------------------------------------------
	
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	DPS_Radio()
	: radio_ ( 0 ),
	debug_ ( 0 )
	{}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	~DPS_Radio()
	{
		disable_radio();
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	init( void )
	{
		if ( enable_radio() != SUCCESS )
			return ERR_UNSPEC;
		
		return SUCCESS;
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	destruct( void )
	{
		return disable_radio();
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	enable_radio( void )
	{
		#ifdef DPS_RADIO_DEBUG
		debug().debug( "DPS Radio: initialization at %llx", (long long unsigned)(radio().id()) );
		#endif
		
		radio().enable_radio();
		
		radio().template reg_recv_callback<self_type, &self_type::receive>( this );
		
		//Start the periodic timer with an extra 2 sec delay in order to establish initial connections
		timer().template set_timer<self_type, &self_type::general_periodic_timer>( DPS_GENERAL_TIMER_FREQUENCY + 2000, this, NULL );
		
		
		//NOTE only for TEST
		disabled_ = false;
		if( id() == 0 )
			timer().template set_timer<self_type, &self_type::turn_off>( 8000, this, NULL );
		
		
		return SUCCESS;
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	disable_radio( void )
	{
		#ifdef DPS_RADIO_DEBUG
		debug().debug( "DPS Radio: Disable" );
		#endif
		
		if( radio().disable_radio() != SUCCESS )
			return ERR_UNSPEC;
		radio().template unreg_recv_callback(callback_id_);
		
		return SUCCESS;
	}
	
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	void
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	generate_connection_key( Connection_list_iterator connection )
	{
		//Key derivation
		memcpy(connection->key, DPS_REQUEST_KEY, 16);
		radio_node_id_t x, y;
		
		//Use a fixed order of the two addresses
		if( connection->partner_MAC > id() )
		{
			x = connection->partner_MAC;
			y = id();
		}
		else
		{
			x = id();
			y = connection->partner_MAC;
		}
		memcpy(&(connection->key[0]), &x, sizeof(radio_node_id_t));
		memcpy(&(connection->key[8]), &y, sizeof(radio_node_id_t));
		
		// XOR it with the connection_nonce
		for (uint8_t i=0; i<=12; i=i+4) {
			connection->key[i+0] ^= ((connection->connection_nonce >> 24) & 0xFF);
			connection->key[i+1] ^= ((connection->connection_nonce >> 16) & 0xFF);
			connection->key[i+2] ^= ((connection->connection_nonce >> 8) & 0xFF);
			connection->key[i+3] ^= ((connection->connection_nonce >> 0) & 0xFF);
		}
	}
	
#if DPS_FOOTER > 0
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	void
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	calculate_checksum_for_buffer( size_t length, block_data_t* buffer, Connection_list_iterator connection, block_data_t* MAC, bool use_request_key )
	{
		memset(MAC,0,4);
		
		//XOR the byte of the buffer into the 4-byte-long MAC buffer
		for ( uint8_t i = 0; i < length; i++ ) 
		{
			MAC[i%4] ^= buffer[i];
		}
#if DPS_FOOTER == 1
		//XOR the buffer with the connection key
		for ( uint8_t i = 0; i < 16; i++ ) 
		{
			//Use the request key for the DISCOVERY and ADVERTISE messages
			if( use_request_key )
				MAC[i%4] ^= DPS_REQUEST_KEY[i];
			else
				MAC[i%4] ^= connection->key[i];
		}
#else
#error Implement AES based auth.
#endif
	}
#endif //DPS_FOOTER > 0
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	send_connection_message( radio_node_id_t destination, uint8_t type, Connection_list_iterator connection )
	{
		
		//NOTE only for TEST
		if( disabled_ )
			return ERR_UNSPEC;
		
		//Create a packet, no fragmentation
		DPS_Packet_t packet( type, false );
		block_data_t* payload_act_pointer=packet.get_payload();
		
		//Set the common fields
		packet.set_pid( connection->Pid );
		packet.set_counter( connection->client_counter );
		
		bool use_requst_key_for_checksum = false;
		
		if( type == DPS_Packet_t::DPS_TYPE_DISCOVERY || type == DPS_Packet_t::DPS_TYPE_ADVERTISE )
		{
			use_requst_key_for_checksum = true;
			
			//TODO handle filters
// 			uint8_t number_of_filters = 0;
			payload_act_pointer[0] = 0; //number_of_filters;
			
			payload_act_pointer++;
		}
		else if( type == DPS_Packet_t::DPS_TYPE_CONNECT_REQUEST )
		{
			connection->connection_nonce = rand()() % (0xFFFFFFFF);
			bitwise_write<OsModel, block_data_t, uint32_t>( payload_act_pointer, connection->connection_nonce, 0, 32 );
			payload_act_pointer += 4;
			
			//Generate the key for the connection based on the nonce
			generate_connection_key( connection );
		}
		else if( type == DPS_Packet_t::DPS_TYPE_CONNECT_ALLOW )
		{
			connection->server_counter = rand()() % (0xFFFFFFFF);
			bitwise_write<OsModel, block_data_t, uint32_t>( payload_act_pointer, connection->server_counter, 0, 32 );
			payload_act_pointer += 4;
		}
		else if( type == DPS_Packet_t::DPS_TYPE_CONNECT_FINISH )
		{
			bitwise_write<OsModel, block_data_t, uint32_t>( payload_act_pointer, connection->server_counter, 0, 32 );
			payload_act_pointer += 4;
		}
		else if( type == DPS_Packet_t::DPS_TYPE_CONNECT_ABORT )
		{
			uint32_t dummy = rand()() % (0xFFFFFFFF);
			bitwise_write<OsModel, block_data_t, uint32_t>( payload_act_pointer, dummy, 0, 32 );
			payload_act_pointer += 4;
		}
		else if( type == DPS_Packet_t::DPS_TYPE_HARTBEAT )
		{
			if( protocol_list_[connection->Pid].server )
			{
				packet.set_counter( connection->server_counter );
				connection->server_counter++;
			}
			else
			{
				connection->client_counter++;
			}
		}
		else
		{
			#ifdef DPS_RADIO_DEBUG
			debug().debug( "DPS: Error, unsupported type code for connection message");
			#endif
		}
		
		//Calculate the full length
		packet.length += payload_act_pointer - packet.get_payload();
		
		
		//If the footer is enabled then calculate the checksum and copy it to the end of the message
#if DPS_FOOTER > 0
		uint8_t MAC[4];
		calculate_checksum_for_buffer( packet.length, packet.buffer, connection, MAC, use_requst_key_for_checksum );
		//Free place is always reserved for these 4 bytes
		memcpy( payload_act_pointer, MAC, 4 );
		//payload += 4; - won't be used any more
		packet.length += 4;
// 		debug().debug( "DPS: %x %x %x %x length: %i", MAC[0], MAC[1], MAC[2], MAC[3], packet.length);
#endif
		
		
		#ifdef DPS_RADIO_DEBUG
		debug().debug( "DPS: send from %llx to %llx, packet type: %i", (long long unsigned)(radio().id()), (long long unsigned)(destination), type);
		#endif
		
// 		packet.set_debug( *debug_ );
// 		packet.print_header();

		radio().send( destination, packet.length, packet.buffer );
		
		return SUCCESS;
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	int
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	send( node_id_t destination, uint16_t length, block_data_t *data )
	{
		//NOTE only for TEST
		if( disabled_ )
			return ERR_UNSPEC;
		
		for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
		{
			if( it->Pid == destination.Pid && it->connection_status == connection_type::CONNECTED )
			{
				bool fragmentation = false;
				
				//Check the size of the data: Max message size vs. header + payload + footer
				if( Radio::MAX_MESSAGE_LENGTH < length + DPS_Packet_t::DPS_RPC_HEADER_SIZE + DPS_Packet_t::DPS_FOOTER_SIZE )
					fragmentation = true;
					
				//Create a packet with or without fragmentation
				DPS_Packet_t packet( DPS_Packet_t::DPS_TYPE_RPC_REQUEST, fragmentation );
				block_data_t* payload_act_pointer=packet.get_payload();
				
				packet.set_pid( destination.Pid );
				packet.set_fid( destination.Fid );
				packet.set_ack_flag( destination.ack_required );
				
				//TODO increment only after all fragments
				if( protocol_list_[packet.pid()].server )
					packet.set_counter( it->server_counter++ );
				else
					packet.set_counter( it->client_counter++ );
				
				//TODO ...
				memcpy( payload_act_pointer, data, length);
				payload_act_pointer += length;
				packet.length += length;
				
#if DPS_FOOTER > 0
				uint8_t MAC[4];
				calculate_checksum_for_buffer( packet.length, packet.buffer, it, MAC, false );
				//Free place is always reserved for these 4 bytes
				memcpy( payload_act_pointer, MAC, 4 );
				//payload_act_pointer += 4; - won't be used any more
				packet.length += 4;
				// 		debug().debug( "DPS: %x %x %x %x length: %i", MAC[0], MAC[1], MAC[2], MAC[3], packet.length);
#endif
				
				#ifdef DPS_RADIO_DEBUG
				debug().debug( "DPS: send RPC from %llx to %llx (%i/%i)", (long long unsigned)(radio().id()), (long long unsigned)(it->partner_MAC), destination.Pid, destination.Fid);
				#endif
				
				radio().send( it->partner_MAC, packet.length, packet.buffer );
				
				return SUCCESS;
			}
		}
		return NO_CONNECTION;
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	void
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	receive( radio_node_id_t from, size_t length, block_data_t *data ) 
	{
		
		DPS_Packet_t packet( length, data );
		block_data_t* payload=packet.get_payload();
		
// 		packet.set_debug( *debug_ );
// 		packet.print_header();
		
		#ifdef DPS_RADIO_DEBUG
		debug().debug( "DPS: Node %llx received from %llx, packet type: %i ", (long long unsigned)(radio().id()), (long long unsigned)(from), packet.type());
		#endif
		//------------ Handle connection message types ------------
		
		if( packet.type() == DPS_Packet_t::DPS_TYPE_DISCOVERY || packet.type() == DPS_Packet_t::DPS_TYPE_ADVERTISE )
		{
#if DPS_FOOTER > 0
			//Checksum validation based on the REQUEST key
			uint8_t MAC[4];
			Connection_list_iterator fake_it;
			calculate_checksum_for_buffer( packet.length-4, packet.buffer, fake_it, MAC, true );
			if( memcmp( MAC, &(packet.buffer[packet.length-4]), 4 ) )
			{
				#ifdef DPS_RADIO_DEBUG
				debug().debug( "DPS: checksum error in DISCOVERY/ADVERTISE packet" );
// 				debug().debug( "(%x %x %x %x) calc (%x %x %x %x)!", packet.buffer[packet.length-4], packet.buffer[packet.length-3], packet.buffer[packet.length-2], packet.buffer[packet.length-1], MAC[0], MAC[1], MAC[2], MAC[3]);
				#endif
				
				return;
			}
#endif
			//Receive a Discovery at a node which provides the protocol as a server
			if( packet.type() == DPS_Packet_t::DPS_TYPE_DISCOVERY && 
				protocol_list_.contains(packet.pid()) && protocol_list_[packet.pid()].server )
			{
				//TODO handle filters
				
				//Check whether there has been a connection already established with this client
				for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
				{
					if( it->Pid == packet.pid() && it->partner_MAC == from )
						return;
				}
				
				//Create the new connection entry and add to the list
				connection_type connection;
				connection.Pid = packet.pid();
				connection.connection_status = connection_type::ADVERTISE_SENT;
				connection.client_counter = packet.counter();
				connection.partner_MAC = from;
				
				Connection_list_iterator it = connection_list_.insert( connection );
				
				if( it == connection_list_.end() )
				{
					#ifdef DPS_RADIO_DEBUG
					debug().debug("DPS: Error, connection list is full");
					#endif
					return;
				}
				
				send_connection_message( from, DPS_Packet_t::DPS_TYPE_ADVERTISE, it );
				return;
			}
			
			//The node can receive this only as a reply
			//TODO LQI based selection --> only store and make decision after some more time
			if( packet.type() == DPS_Packet_t::DPS_TYPE_ADVERTISE &&
				!(protocol_list_[packet.pid()].server) ) //CLIENT CODE
			{
				for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
				{
					if( it->Pid == packet.pid() )
					{
						//Check for duplicate ADVERTISE messages and counter errors
						if( it->partner_MAC != NULL_NODE_ID || it->client_counter != packet.counter() )
							return;
						
						//Reset the timer for the connection
						it->elapsed_time = 0;
						
						it->connection_status = connection_type::CONNECT_SENT;
						it->partner_MAC = from;
						
						//Send a CONNECT to the selected server 
						send_connection_message( from, DPS_Packet_t::DPS_TYPE_CONNECT_REQUEST, it );
						return;
					}
				}
			}
		}
		else if( packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_REQUEST ||
			 packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_ALLOW ||
			 packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_FINISH ||
			 packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_ABORT)
		{
			//Search for the used connection
			for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
			{
				if( ( it->Pid == packet.pid() ) && ( it->partner_MAC == from ) && 
					( it->client_counter == packet.counter() ) && ( it->connection_status != connection_type::CONNECTED ) )
				{
					//CONNECTION_REQUEST must be pre-processed to calculate the key
					if( packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_REQUEST ) //SERVER SIDE
					{
						//TODO check for available resources?
						it->connection_status = connection_type::ALLOW_SENT;
						it->connection_nonce = bitwise_read<OsModel, block_data_t, uint32_t>( payload, 0, 32 );
						
						//Generate the key for the connection based on the nonce (iterator to pointer cast)
						generate_connection_key( it );
					}
					
#if DPS_FOOTER > 0
					//Checksum validation based on the connection key
					uint8_t MAC[4];
					calculate_checksum_for_buffer( packet.length-4, packet.buffer, it, MAC, false );
					if( memcmp( MAC, &(packet.buffer[packet.length-4]), 4 ) )
					{
						#ifdef DPS_RADIO_DEBUG
						debug().debug( "DPS: checksum error in CONNECTION packet" );
// 						debug().debug( "(%x %x %x %x) calc (%x %x %x %x)!", packet.buffer[packet.length-4], packet.buffer[packet.length-3], packet.buffer[packet.length-2], packet.buffer[packet.length-1], MAC[0], MAC[1], MAC[2], MAC[3]);
						#endif
						
						connection_list_.erase( it );
						return;
					}
#endif
					
					//Reset the timer for the connection
					it->elapsed_time = 0;
					
					if( packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_REQUEST ) //SERVER SIDE
					{
						send_connection_message( from, DPS_Packet_t::DPS_TYPE_CONNECT_ALLOW, it );
					}
					else if( packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_ALLOW ) //CLIENT SIDE
					{
						it->connection_status = connection_type::CONNECTED;
						it->server_counter = bitwise_read<OsModel, block_data_t, uint32_t>( payload, 0, 32 );
						
						#ifdef DPS_RADIO_DEBUG
						debug().debug( "DPS: Client (%llx) connected to %llx, protocol: %i", (long long unsigned)(radio().id()), (long long unsigned)(from), it->Pid);
						debug().debug( "DPS parameters CNT_c: %llx, CNT_s: %llx, nonce: %llx", (long long unsigned)(it->client_counter), (long long unsigned)(it->server_counter), (long long unsigned)(it->connection_nonce) );
						#endif
						
						send_connection_message( from, DPS_Packet_t::DPS_TYPE_CONNECT_FINISH, it );
					}
					else if( packet.type() == DPS_Packet_t::DPS_TYPE_CONNECT_FINISH ) //SERVER SIDE
					{
						it->connection_status = connection_type::CONNECTED;
						
						#ifdef DPS_RADIO_DEBUG
						debug().debug( "DPS: Server (%llx) connected to %llx, protocol: %i", (long long unsigned)(radio().id()), (long long unsigned)(from), it->Pid);
						debug().debug( "DPS parameters CNT_c: %llx, CNT_s: %llx, nonce: %llx", (long long unsigned)(it->client_counter), (long long unsigned)(it->server_counter), (long long unsigned)(it->connection_nonce) );
						#endif
					}
					else //DPS_Packet_t::DPS_TYPE_CONNECT_ABORT
					{
						connection_list_.erase( it );
					}
					return;
				}
			}
			
			#ifdef DPS_RADIO_DEBUG
			debug().debug( "DPS: Error, (%llx) received connection message from %llx without matching conn. entry!", (long long unsigned)(radio().id()), (long long unsigned)(from));
			#endif
		}
		//Messages with CONNECTED status
		else
		{
			for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
			{
				if( ( it->Pid == packet.pid() ) && ( it->partner_MAC == from ) )
				{
#if DPS_FOOTER > 0
					//Checksum validation
					uint8_t MAC[4];
					calculate_checksum_for_buffer( packet.length-4, packet.buffer, it, MAC, false );
					if( memcmp( MAC, &(packet.buffer[packet.length-4]), 4 ) )
					{
						#ifdef DPS_RADIO_DEBUG
						debug().debug( "DPS: checksum error in CONNECTED packet" );
// 						debug().debug( "(%x %x %x %x) calc (%x %x %x %x)!", packet.buffer[packet.length-4], packet.buffer[packet.length-3], packet.buffer[packet.length-2], packet.buffer[packet.length-1], MAC[0], MAC[1], MAC[2], MAC[3]);
						#endif
						return;
					}
#endif
					if( packet.type() == DPS_Packet_t::DPS_TYPE_HARTBEAT )
					{
						//Server side, check for the counter against replay attack
						if( ( it->client_counter == packet.counter() ) && ( protocol_list_[packet.pid()].server ) )
						{
// 							debug().debug( "DPS: HB ok at %llx", (long long unsigned)(radio().id()) );
							it->elapsed_time = 0;
							it->client_counter++;
							return;
						}
						//Client side, check for the counter against replay attack
						else if( ( it->server_counter == packet.counter() ) && !( protocol_list_[packet.pid()].server ) )
						{
// 							debug().debug( "DPS: HB ok at %llx", (long long unsigned)(radio().id()) );
							it->elapsed_time = 0;
							it->server_counter++;
							return;
						}
					}
					else if( packet.type() == DPS_Packet_t::DPS_TYPE_RPC_ACK )
					{
						//TODO handle...
					}
					else if( packet.type() == DPS_Packet_t::DPS_TYPE_RPC_REQUEST )
					{
						//Send back an ACK if it is requested by the sender
						if( packet.ack_flag() )
						{
							DPS_Packet_t ack_packet( DPS_Packet_t::DPS_TYPE_RPC_ACK, false );
							
							if( packet.fragmentation_flag() )
							{
								//Modify the header by hand because it requires less resources
								ack_packet.set_fragmentation_flag( 1 );
								ack_packet.length += DPS_Packet_t::DPS_FRAGMENTATION_HEADER_SIZE;
								ack_packet.payload_position += DPS_Packet_t::DPS_FRAGMENTATION_HEADER_SIZE;
								ack_packet.set_fragmentation_header(packet.fragmentation_header_length(), packet.fragmentation_header_shift());
							}
							
							ack_packet.set_pid( packet.pid() );
							ack_packet.set_fid( packet.fid() );
							ack_packet.set_counter( packet.counter() );
							
#if DPS_FOOTER > 0
							calculate_checksum_for_buffer( ack_packet.length, ack_packet.buffer, it, MAC, false );
							//No payload
							memcpy( ack_packet.buffer + ack_packet.payload_position, MAC, 4 );
							ack_packet.length += 4;
#endif
							radio().send( it->partner_MAC, ack_packet.length, ack_packet.buffer );
						}
						
						//TODO check the counter values, collect fragments...
						
						//get buffer from the handler
						app_buffer = (protocol_list_[packet.pid()].buffer_handler_delegate)( NULL, length, true );
						
						
						node_id_t source;
						source.Pid = packet.pid();
						source.Fid = packet.fid();
						uint16_t payload_length = packet.length - packet.payload_position;
						
						//TODO only after full packets
						if( protocol_list_[packet.pid()].server )
							it->client_counter++;
						else
							it->server_counter++;
						
						//Copy the payload
						memcpy( app_buffer, packet.buffer + packet.payload_position, payload_length );
						
						#if DPS_FOOTER > 0
						payload_length -= 4;
						#endif
						
						(protocol_list_[packet.pid()].rpc_handler_delegate)( source, payload_length, app_buffer );
					}
					else
					{
						//TODO process RPC messages here
					}
				}
			}
		}
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P,
		typename Timer_P,
		typename Rand_P>
	void
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	send_DISCOVERY( void* n_in )
	{
		int n = (int)n_in;
		
		Connection_list_iterator it = connection_list_.begin() + n;
		
		send_connection_message( Radio::BROADCAST_ADDRESS, DPS_Packet_t::DPS_TYPE_DISCOVERY, it );
		
		//NOTE may not the expected
		if( it->connection_status == connection_type::SENDING_DISCOVERY )
		{
			it->elapsed_time += DPS_TIMER_DISCOVERY_FREQUENCY;
			timer().template set_timer<self_type, &self_type::send_DISCOVERY>( DPS_TIMER_DISCOVERY_FREQUENCY, this, n_in );
		}
	}
	
	// -----------------------------------------------------------------------
	template<typename OsModel_P,
	typename Radio_P,
	typename Debug_P,
	typename Timer_P,
	typename Rand_P>
	void
	DPS_Radio<OsModel_P, Radio_P, Debug_P, Timer_P, Rand_P>::
	general_periodic_timer( void* )
	{
		for( Connection_list_iterator it = connection_list_.begin(); it != connection_list_.end(); ++it )
		{
			it->elapsed_time += DPS_GENERAL_TIMER_FREQUENCY;
			
			//CONNECTED and ( HARTBEAT_TIMEOUT <= elapsed_time < DELETE_TIMEOUT )
			if(( it->connection_status == connection_type::CONNECTED )
				&& ( it->elapsed_time >= DPS_HARTBEAT_THRESHOLD ) && (it->elapsed_time < DPS_DELETE_CONNECTION_THRESHOLD))
			{
				send_connection_message( it->partner_MAC, DPS_Packet_t::DPS_TYPE_HARTBEAT, it);
				continue;
			}
			
			//For the server (these states are possible only for the server)
			//(ADVERTISE_SENT || ALLOW_SENT) && elapsed_time >= CONNECT_TIMEOUT
			//(CONNECTED)                    && elapsed_time >= DELETE_TIMEOUT
			if( (protocol_list_[it->Pid].server) )
			{
				if( (((it->connection_status == connection_type::ADVERTISE_SENT) || (it->connection_status == connection_type::ALLOW_SENT)) 
							&& (it->elapsed_time >= DPS_CONNECT_TIMEOUT) ) ||
				    (( it->connection_status == connection_type::CONNECTED ) 
							&& (it->elapsed_time >= DPS_DELETE_CONNECTION_THRESHOLD) ))
				{
					#ifdef DPS_RADIO_DEBUG
					debug().debug( "DPS: remove client at (%llx) for %i", (long long unsigned)(radio().id()), it->Pid);
					#endif
					
					connection_list_.erase( it );
					
					//Break the loop if this was the only element
					if( connection_list_.size() == 0 )
						break;
				}
			}
			//For the client (these states are possible only for the client)
			//(CONNECT_SENT) && elapsed_time >= CONNECT_TIMEOUT
			//(CONNECTED)    && elapsed_time >= DELETE_TIMEOUT
			//The SENDING_DISCOVERY state is handled by another timer&function
			else
			{
				if((( it->connection_status == connection_type::CONNECT_SENT )
						&& (it->elapsed_time >= DPS_CONNECT_TIMEOUT) ) ||
				  (( it->connection_status == connection_type::CONNECTED )
						&& (it->elapsed_time >= DPS_DELETE_CONNECTION_THRESHOLD) ))
				{
					#ifdef DPS_RADIO_DEBUG
					debug().debug( "DPS: reDISCOVERY at (%llx) for %i", (long long unsigned)(radio().id()), it->Pid);
					#endif
					
					it->connection_status = connection_type::SENDING_DISCOVERY;
					it->client_counter = rand()() % (0xFFFFFFFF);
					it->server_counter = 0;
					it->partner_MAC = Radio::NULL_NODE_ID;
					it->connection_nonce = 0;
					
					timer().template set_timer<self_type, &self_type::send_DISCOVERY>( DPS_TIMER_DISCOVERY_FREQUENCY, this, (void*)(it-connection_list_.begin()) );
				}
			}
			
		}
		
		timer().template set_timer<self_type, &self_type::general_periodic_timer>( DPS_GENERAL_TIMER_FREQUENCY, this, NULL );
	}
	
}
#endif