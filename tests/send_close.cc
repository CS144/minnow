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

    // this case is similar to SYN + FIN but test explicitly checks if your implementation correctly updates the
    // window size upon receiving a window update from the receiver, even before any acknowledgment has been
    // received. It helps to catch issues where the sender might incorrectly assume a window size = 0 if the recieve
    // function is only updating window size when recieving acknowledgments.
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Window size set before receiving first acknowledgment", cfg };
      //  Set window size without having sent an acknowledgment
      test.execute( Receive { { {}, 1024 } }.without_push() );
      test.execute( ExpectNoSegment {} );
      // Close the stream, expecting that SYN and FIN can be sent together if window size allows
      test.execute( Close {} );
      // Ensure sender can send SYN and FIN flags without having received any acknowledgments
      // must have set window size earlier
      test.execute( ExpectMessage {}.with_syn( true ).with_fin( true ).with_payload_size( 0 ).with_seqno( isn ) );
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
