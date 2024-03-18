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

      TCPSenderTestHarness test { "FIN sent test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN with data", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "hello" }.with_close() );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ).with_data( "hello" ) );
      test.execute( ExpectSeqno { isn + 7 } );
      test.execute( ExpectSeqnosInFlight { 6 } );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "SYN + FIN", cfg };
      test.execute( Receive { { {}, 1024 } }.without_push() );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_syn( true ).with_payload_size( 0 ).with_seqno( isn ).with_fin( true ) );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 2 } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN acked test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 2 } } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN not acked test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN retx test", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { TCPConfig::TIMEOUT_DFLT - 1 } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute( ExpectMessage {}.with_fin( true ).with_seqno( isn + 1 ) );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 2 } } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
