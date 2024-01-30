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
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "If already running, timer stays running when new segment sent", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abc" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( Tick { rto - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Push( "def" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ) );
      test.execute( Tick { 6 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Retransmission still happens when expiration time not hit exactly", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abc" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( Tick { rto - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Push( "def" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ) );
      test.execute( Tick { 200 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Timer restarts on ACK of new data", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abc" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( Tick { rto - 5 } );
      test.execute( Push( "def" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ).with_seqno( isn + 4 ) );
      test.execute( AckReceived { Wrap32 { isn + 4 } }.with_win( 1000 ) );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 2 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ).with_seqno( isn + 4 ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Timer doesn't restart without ACK of new data", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abc" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( Tick { rto - 5 } );
      test.execute( Push( "def" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ).with_seqno( isn + 4 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( Tick { 6 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { rto * 2 - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 8 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "RTO resets on ACK of new data", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abc" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( Tick { rto - 5 } );
      test.execute( Push( "def" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ).with_seqno( isn + 4 ) );
      test.execute( Push( "ghi" ) );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "ghi" ).with_seqno( isn + 7 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( Tick { 6 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { rto * 2 - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 5 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { rto * 4 - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 4 } }.with_win( 1000 ) );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 2 } );
      test.execute( ExpectMessage {}.with_payload_size( 3 ).with_data( "def" ).with_seqno( isn + 4 ) );
      test.execute( ExpectNoSegment {} );
    }
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      const string nicechars = "abcdefghijklmnopqrstuvwxyz";
      string bigstring;
      for ( unsigned int i = 0; i < TCPConfig::DEFAULT_CAPACITY; i++ ) {
        bigstring.push_back( nicechars.at( rd() % nicechars.size() ) );
      }

      const size_t window_size = uniform_int_distribution<uint16_t> { 50000, 63000 }( rd );

      TCPSenderTestHarness test { "fill_window() correctly fills a big window", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( window_size ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { bigstring } );

      for ( unsigned int i = 0; i + TCPConfig::MAX_PAYLOAD_SIZE < min( bigstring.size(), window_size );
            i += TCPConfig::MAX_PAYLOAD_SIZE ) {
        const size_t expected_size = min( TCPConfig::MAX_PAYLOAD_SIZE, min( bigstring.size(), window_size ) - i );
        test.execute( ExpectMessage {}
                        .with_no_flags()
                        .with_payload_size( expected_size )
                        .with_data( bigstring.substr( i, expected_size ) )
                        .with_seqno( isn + 1 + i ) );
      }
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Retransmit a FIN-containing segment same as any other", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "abc" }.with_close() );
      test.execute(
        ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ).with_fin( true ) );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 2 } );
      test.execute(
        ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ).with_fin( true ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Retransmit a FIN-only segment same as any other", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { "abc" } );
      test.execute(
        ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ).with_no_flags() );
      test.execute( Close {} );
      test.execute( ExpectMessage {}.with_payload_size( 0 ).with_seqno( isn + 4 ).with_fin( true ) );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { isn + 4 }.with_win( 1000 ) );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 2 } );
      test.execute( ExpectMessage {}.with_payload_size( 0 ).with_seqno( isn + 4 ).with_fin( true ) );
      test.execute( Tick { 2 * rto - 5 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 10 } );
      test.execute( ExpectMessage {}.with_payload_size( 0 ).with_seqno( isn + 4 ).with_fin( true ) );
      test.execute( ExpectSeqno { isn + 5 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Don't add FIN if this would make the segment exceed the receiver's window",
                                  cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( Push( "abc" ).with_close() );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 3 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ).with_no_flags() );
      test.execute( ExpectSeqno { isn + 4 } );
      test.execute( ExpectSeqnosInFlight { 3 } );
      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 2 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 3 } }.with_win( 1 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 4 } }.with_win( 1 ) );
      test.execute( ExpectMessage {}.with_payload_size( 0 ).with_seqno( isn + 4 ).with_fin( true ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Don't send FIN by itself if the window is full", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( Push( "abc" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 3 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 3 ).with_data( "abc" ).with_seqno( isn + 1 ).with_no_flags() );
      test.execute( ExpectSeqno { isn + 4 } );
      test.execute( ExpectSeqnosInFlight { 3 } );
      test.execute( Close {} );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 2 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 3 } }.with_win( 1 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 4 } }.with_win( 1 ) );
      test.execute( ExpectMessage {}.with_payload_size( 0 ).with_seqno( isn + 4 ).with_fin( true ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      const string nicechars = "abcdefghijklmnopqrstuvwxyz";
      string bigstring;
      for ( unsigned int i = 0; i < TCPConfig::MAX_PAYLOAD_SIZE; i++ ) {
        bigstring.push_back( nicechars.at( rd() % nicechars.size() ) );
      }

      TCPSenderTestHarness test { "MAX_PAYLOAD_SIZE limits payload only", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( Push { string( bigstring ) }.with_close() );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 40000 ) );
      test.execute( ExpectMessage {}
                      .with_payload_size( TCPConfig::MAX_PAYLOAD_SIZE )
                      .with_data( string( bigstring ) )
                      .with_seqno( isn + 1 )
                      .with_fin( true ) );
      test.execute( ExpectSeqno { isn + 2 + TCPConfig::MAX_PAYLOAD_SIZE } );
      test.execute( ExpectSeqnosInFlight { 1 + TCPConfig::MAX_PAYLOAD_SIZE } );
      test.execute( AckReceived( isn + 2 + TCPConfig::MAX_PAYLOAD_SIZE ) );
      test.execute( ExpectSeqno { isn + 2 + TCPConfig::MAX_PAYLOAD_SIZE } );
      test.execute( ExpectSeqnosInFlight { 0 } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test {
        "When filling window, treat a '0' window size as equal to '1' but don't back off RTO", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( Push( "abc" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 0 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( Close {} );
      test.execute( ExpectNoSegment {} );

      for ( unsigned int i = 0; i < 5; i++ ) {
        test.execute( Tick { rto - 1 } );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 } );
        test.execute(
          ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );
      }

      test.execute( AckReceived { isn + 2 }.with_win( 0 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "b" ).with_seqno( isn + 2 ).with_no_flags() );

      for ( unsigned int i = 0; i < 5; i++ ) {
        test.execute( Tick { rto - 1 } );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 } );
        test.execute(
          ExpectMessage {}.with_payload_size( 1 ).with_data( "b" ).with_seqno( isn + 2 ).with_no_flags() );
      }

      test.execute( AckReceived { isn + 3 }.with_win( 0 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "c" ).with_seqno( isn + 3 ).with_no_flags() );

      for ( unsigned int i = 0; i < 5; i++ ) {
        test.execute( Tick { rto - 1 } );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 } );
        test.execute(
          ExpectMessage {}.with_payload_size( 1 ).with_data( "c" ).with_seqno( isn + 3 ).with_no_flags() );
      }

      test.execute( AckReceived { isn + 4 }.with_win( 0 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 0 ).with_data( "" ).with_seqno( isn + 4 ).with_fin( true ) );

      for ( unsigned int i = 0; i < 5; i++ ) {
        test.execute( Tick { rto - 1 } );
        test.execute( ExpectNoSegment {} );
        test.execute( Tick { 1 } );
        test.execute(
          ExpectMessage {}.with_payload_size( 0 ).with_data( "" ).with_seqno( isn + 4 ).with_fin( true ) );
      }
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Unlike a zero-size window, a full window of nonzero size should be respected",
                                  cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( Push( "abc" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );
      test.execute( ExpectSeqno { isn + 2 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( Tick { rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );

      test.execute( Close {} );

      test.execute( Tick { 2 * rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );

      test.execute( Tick { 4 * rto - 1 } );
      test.execute( ExpectNoSegment {} );
      test.execute( Tick { 1 } );
      test.execute(
        ExpectMessage {}.with_payload_size( 1 ).with_data( "a" ).with_seqno( isn + 1 ).with_no_flags() );

      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 3 ) );
      test.execute(
        ExpectMessage {}.with_payload_size( 2 ).with_data( "bc" ).with_seqno( isn + 2 ).with_fin( true ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      const size_t rto = uniform_int_distribution<uint16_t> { 30, 10000 }( rd );
      cfg.isn = isn;
      cfg.rt_timeout = rto;

      TCPSenderTestHarness test { "Repeated ACKs and outdated ACKs are harmless", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push( "abcdefg" ) );
      test.execute( ExpectMessage {}.with_payload_size( 7 ).with_data( "abcdefg" ).with_seqno( isn + 1 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( ExpectNoSegment {} );
      test.execute( Push( "ijkl" ).with_close() );
      test.execute(
        ExpectMessage {}.with_payload_size( 4 ).with_data( "ijkl" ).with_seqno( isn + 8 ).with_fin( true ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 8 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 12 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 12 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 12 } }.with_win( 1000 ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 1000 ) );
      test.execute( Tick { 5 * rto } );
      test.execute(
        ExpectMessage {}.with_payload_size( 4 ).with_data( "ijkl" ).with_seqno( isn + 8 ).with_fin( true ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived( Wrap32 { isn + 13 } ).with_win( 1000 ) );
      test.execute( AckReceived( Wrap32 { isn + 1 } ).with_win( 1000 ) );
      test.execute( Tick { 5 * rto } );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqnosInFlight { 0 } );
    }

    // test credit: DD152
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;
      const string nicechars = "abcdefghijklmnopqrstuvwxyz";
      string bigstring;
      for ( unsigned int i = 0; i < TCPConfig::DEFAULT_CAPACITY; i++ ) {
        bigstring.push_back( nicechars.at( rd() % nicechars.size() ) );
      }
      // max window size
      const size_t window_size = 65535;
      TCPSenderTestHarness test { "Only the last segment has FIN", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( window_size ) );
      test.execute( ExpectSeqno { isn + 1 } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( Push { bigstring }.with_close() );
      unsigned int i = 0;
      while ( ( i + TCPConfig::MAX_PAYLOAD_SIZE ) < bigstring.size() ) {
        test.execute( ExpectMessage {}
                        .with_no_flags()
                        .with_payload_size( TCPConfig::MAX_PAYLOAD_SIZE )
                        .with_data( bigstring.substr( i, TCPConfig::MAX_PAYLOAD_SIZE ) )
                        .with_seqno( isn + 1 + i ) );
        i += TCPConfig::MAX_PAYLOAD_SIZE;
      }
      test.execute( ExpectMessage {}
                      .with_fin( true )
                      .with_payload_size( bigstring.size() - i )
                      .with_data( bigstring.substr( i, bigstring.size() - 1 ) )
                      .with_seqno( isn + 1 + i ) );
      test.execute( ExpectNoSegment {} );
      test.execute( ExpectSeqno { isn + 2 + bigstring.size() } );
      test.execute( ExpectSeqnosInFlight { bigstring.size() + 1 } );
      test.execute( AckReceived { isn + 2 + bigstring.size() } );
      test.execute( ExpectSeqnosInFlight { 0 } );
      test.execute( ExpectSeqno { isn + 2 + bigstring.size() } );
    }

    // Sender must set RST iff stream has suffered an error
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;
      TCPSenderTestHarness test { "Stream error -> RST flag", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ) );
      test.execute( SetError {} );
      test.execute( ExpectReset { true } );
    }

    // Sender must set RST iff stream has suffered an error
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;
      TCPSenderTestHarness test { "Stream error -> RST flag (non-empty segment)", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ) );
      test.execute( AckReceived { Wrap32 { isn + 1 } }.with_win( 100 ) );
      test.execute( SetError {} );
      test.execute( Push { "hello" } );
      test.execute( ExpectMessage {}.with_no_flags().with_rst( true ).with_seqno( isn + 1 ) );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "RST flag -> stream error", cfg };
      test.execute( Receive { TCPReceiverMessage { .RST = true } } );
      test.execute( HasError { true } );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
