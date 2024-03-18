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

      TCPSenderTestHarness test { "Initial receiver advertised window is respected", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 4 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "abcdefg" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "abcd" ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Immediate window is respected", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 6 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "abcdefg" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "abcdef" ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      const size_t MIN_WIN = 5;
      const size_t MAX_WIN = 100;
      const size_t N_REPS = 1000;
      for ( size_t i = 0; i < N_REPS; ++i ) {
        const size_t len = MIN_WIN + rd() % ( MAX_WIN - MIN_WIN );
        TCPConfig cfg;
        const Wrap32 isn( rd() );
        cfg.isn = isn;

        TCPSenderTestHarness test { "Window " + to_string( i ), cfg };
        test.execute( Push {} );
        test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
        test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( len ) );
        test.execute( ExpectNoSegment {} );
        test.execute( Push { string( 2 * N_REPS, 'a' ) } );
        test.execute( ExpectMessage {}.with_no_flags().with_payload_size( len ) );
        test.execute( ExpectNoSegment {} );
      }
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Window growth is exploited", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 4 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "0123456789" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "0123" ) );
      test.execute( AckReceived { Wrap32 { isn + 5 } }.with_win( 5 ) );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "45678" ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN flag occupies space in window", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 7 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "1234567" } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "1234567" ) );
      test.execute( ExpectNoSegment {} ); // window is full
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1 ) );
      test.execute( ExpectMessage {}.with_fin( true ).with_data( "" ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "FIN flag occupies space in window (part II)", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 7 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "1234567" } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "1234567" ) );
      test.execute( ExpectNoSegment {} ); // window is full
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 8 ) );
      test.execute( ExpectMessage {}.with_fin( true ).with_data( "" ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Piggyback FIN in segment when space is available", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 3 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "1234567" } );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "123" ) );
      test.execute( ExpectNoSegment {} ); // window is full
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 8 ) );
      test.execute( ExpectMessage {}.with_fin( true ).with_data( "4567" ) );
      test.execute( ExpectNoSegment {} );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
