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
      cfg.isn = isn;

      TCPSenderTestHarness test { "Repeat ACK is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( Push { "a" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "a" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Old ACK is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( Push { "a" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "a" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 2 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "b" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "b" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    // credit for test: Jared Wasserman (2020)
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Impossible ackno (beyond next seqno) is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 1000 ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( HasError { false } );
    }

    // disable controversial test for 2024
#if 0
    // test credit: Ammar Ratnani
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "A partially acknowledged segment is still fully in flight", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1024 ) );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "ab" } );
      test.execute( ExpectMessage {}.with_no_flags().with_payload_size( 2 ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqnosInFlight { 2 } );
      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 1000 ) );
      test.execute( ExpectSeqnosInFlight { 2 } );
    }
#endif
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
