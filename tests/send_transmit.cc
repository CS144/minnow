#include "random.hh"
#include "sender_test_harness.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main()
{
  try {
    auto rd = get_random_engine();

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test { "Three short writes", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "ab" } );
      test.execute( ExpectMessage {}.with_data( "ab" ).with_seqno( isn + 1 ) );
      test.execute( Push { "cd" } );
      test.execute( ExpectMessage {}.with_data( "cd" ).with_seqno( isn + 3 ) );
      test.execute( Push { "abcd" } );
      test.execute( ExpectMessage {}.with_data( "abcd" ).with_seqno( isn + 5 ) );
      test.execute( ExpectSeqno { isn + 9 } );
      test.execute( ExpectSeqnosInFlight { 8 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test { "Many short writes, continuous acks", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      constexpr uint32_t max_block_size = 10;
      constexpr uint32_t n_rounds = 10000;
      size_t bytes_sent = 0;
      for ( uint32_t i = 0; i < n_rounds; ++i ) {
        string data;
        const uint32_t block_size = uniform_int_distribution<uint32_t> { 1, max_block_size }( rd );
        for ( uint32_t j = 0; j < block_size; ++j ) {
          const uint8_t c = 'a' + ( ( i + j ) % 26 );
          data.push_back( static_cast<char>( c ) );
        }
        test.execute( ExpectSeqno { isn + static_cast<uint32_t>( bytes_sent ) + 1 } );
        test.execute( Push { data } );
        bytes_sent += block_size;
        test.execute( ExpectSeqnosInFlight { block_size } );
        test.execute( ExpectMessage {}
                        .with_seqno( isn + 1 + static_cast<uint32_t>( bytes_sent - block_size ) )
                        .with_data( move( data ) ) );
        test.execute( ExpectNoSegment {} );
        test.execute( AckReceived { Wrap32 { isn + 1 + static_cast<uint32_t>( bytes_sent ) } } );
      }
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test { "Many short writes, ack at end", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { isn + 1 }.with_win( 65000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      constexpr uint32_t max_block_size = 10;
      constexpr uint32_t n_rounds = 1000;
      size_t bytes_sent = 0;
      for ( uint32_t i = 0; i < n_rounds; ++i ) {
        string data;
        const uint32_t block_size = uniform_int_distribution<uint32_t> { 1, max_block_size }( rd );
        for ( uint32_t j = 0; j < block_size; ++j ) {
          const uint8_t c = 'a' + ( ( i + j ) % 26 );
          data.push_back( static_cast<char>( c ) );
        }
        test.execute( ExpectSeqno { Wrap32 { isn + static_cast<uint32_t>( bytes_sent ) + 1 } } );
        test.execute( Push( string( data ) ) );
        bytes_sent += block_size;
        test.execute( ExpectSeqnosInFlight { bytes_sent } );
        test.execute( ExpectMessage {}
                        .with_seqno( isn + 1 + static_cast<uint32_t>( bytes_sent - block_size ) )
                        .with_data( move( data ) ) );
        test.execute( ExpectNoSegment {} );
      }
      test.execute( ExpectSeqnosInFlight { bytes_sent } );
      test.execute( AckReceived { Wrap32 { isn + 1 + static_cast<uint32_t>( bytes_sent ) } } );
      test.execute( ExpectSeqnosInFlight { 0 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test { "Window filling", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 3 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "01234567" ) );
      test.execute( ExpectSeqnosInFlight { 3 } );
      test.execute( ExpectMessage {}.with_data( "012" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { Wrap32 { isn + 1 + 3 } } );
      test.execute( AckReceived { Wrap32 { isn + 1 + 3 } }.with_win( 3 ) );
      test.execute( ExpectSeqnosInFlight { 3 } );
      test.execute( ExpectMessage {}.with_data( "345" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { Wrap32 { isn + 1 + 6 } } );
      test.execute( AckReceived { Wrap32 { isn + 1 + 6 } }.with_win( 3 ) );
      test.execute( ExpectSeqnosInFlight { 2 } );
      test.execute( ExpectMessage {}.with_data( "67" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { Wrap32 { isn + 1 + 8 } } );
      test.execute( AckReceived { Wrap32 { isn + 1 + 8 } }.with_win( 3 ) );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test { "Immediate writes respect the window", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 3 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "01" ) );
      test.execute( ExpectSeqnosInFlight { 2 } );
      test.execute( ExpectMessage {}.with_data( "01" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { Wrap32 { isn + 1 + 2 } } );
      test.execute( Push( "23" ) );
      test.execute( ExpectSeqnosInFlight { 3 } );
      test.execute( ExpectMessage {}.with_data( "2" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { Wrap32 { isn + 1 + 3 } } );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
