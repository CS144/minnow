#include "sender_test_harness.hh"

#include "random.hh"

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

      TCPSenderTestHarness test { "SYN sent after first push", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "SYN acked test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { isn + 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqnosInFlight { 0 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "SYN -> wrong ack test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { isn } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqnosInFlight { 1 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "SYN acked, data", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { isn + 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "abcdefgh" } );
      test.execute( Tick { 1 } );
      test.execute( ExpectMessage {}.with_seqno( isn + 1 ).with_data( "abcdefgh" ) );
      test.execute( ExpectSeqno { isn + 9 } );
      test.execute( ExpectSeqnosInFlight { 8 } );
      test.execute( AckReceived { isn + 9 } );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( ExpectSeqno { isn + 9 } );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
