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
      const uint16_t retx_timeout = uniform_int_distribution<uint16_t> { 10, 10000 }( rd );
      cfg.fixed_isn = isn;
      cfg.rt_timeout = retx_timeout;

      TCPSenderTestHarness test { "Retx SYN twice at the right times, then ack", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( Tick { retx_timeout - 1U } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      // Wait twice as long b/c exponential back-off
      test.execute( Tick { 2 * retx_timeout - 1U } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const uint16_t retx_timeout = uniform_int_distribution<uint16_t> { 10, 10000 }( rd );
      cfg.fixed_isn = isn;
      cfg.rt_timeout = retx_timeout;

      TCPSenderTestHarness test { "Retx SYN until too many retransmissions", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      for ( size_t attempt_no = 0; attempt_no < TCPConfig::MAX_RETX_ATTEMPTS; attempt_no++ ) {
        test.execute( Tick { ( retx_timeout << attempt_no ) - 1U }.with_max_retx_exceeded( false ) );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 }.with_max_retx_exceeded( false ) );
        test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
        test.execute( ExpectSeqno { isn + 1 } );
        test.execute( ExpectSeqnosInFlight { 1 } );
      }
      test.execute(
        Tick { ( retx_timeout << TCPConfig::MAX_RETX_ATTEMPTS ) - 1U }.with_max_retx_exceeded( false ) );
      test.execute( Tick { 1 }.with_max_retx_exceeded( true ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const uint16_t retx_timeout = uniform_int_distribution<uint16_t> { 10, 10000 }( rd );
      cfg.fixed_isn = isn;
      cfg.rt_timeout = retx_timeout;

      TCPSenderTestHarness test { "Send some data, the retx and succeed, then retx till limit", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( Push { "abcd" } );
      test.execute( ExpectMessage {}.with_payload_size( 4 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 5 } } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "efgh" } );
      test.execute( ExpectMessage {}.with_payload_size( 4 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { retx_timeout }.with_max_retx_exceeded( false ) );
      test.execute( ExpectMessage {}.with_payload_size( 4 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 9 } } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "ijkl" } );
      test.execute( ExpectMessage {}.with_payload_size( 4 ).with_seqno( isn + 9 ) );
      for ( size_t attempt_no = 0; attempt_no < TCPConfig::MAX_RETX_ATTEMPTS; attempt_no++ ) {
        test.execute( Tick { ( retx_timeout << attempt_no ) - 1U }.with_max_retx_exceeded( false ) );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 }.with_max_retx_exceeded( false ) );
        test.execute( ExpectMessage {}.with_payload_size( 4 ).with_seqno( isn + 9 ) );
        test.execute( ExpectSeqnosInFlight { 4 } );
      }
      test.execute(
        Tick { ( retx_timeout << TCPConfig::MAX_RETX_ATTEMPTS ) - 1U }.with_max_retx_exceeded( false ) );
      test.execute( Tick { 1 }.with_max_retx_exceeded( true ) );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
