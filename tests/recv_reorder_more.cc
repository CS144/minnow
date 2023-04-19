#include "random.hh"
#include "receiver_test_harness.hh"
#include "tcp_config.hh"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

static constexpr unsigned NREPS = 64;

void do_test_1( const TCPConfig& cfg, default_random_engine& rd )
{
  const Wrap32 rx_isn( rd() );
  TCPReceiverTestHarness test_1 { "non-overlapping out-of-order segments", cfg.recv_capacity };
  test_1.execute( SegmentArrives {}.with_seqno( rx_isn ).with_syn() );
  vector<tuple<size_t, size_t>> seq_size;
  size_t datalen = 0;
  while ( datalen < cfg.recv_capacity ) {
    const size_t size = min( 1 + ( static_cast<size_t>( rd() ) % ( TCPConfig::MAX_PAYLOAD_SIZE - 1 ) ),
                             cfg.recv_capacity - datalen );
    seq_size.emplace_back( datalen, size );
    datalen += size;
  }
  shuffle( seq_size.begin(), seq_size.end(), rd );

  string d( datalen, 0 );
  generate( d.begin(), d.end(), [&] { return rd(); } );

  uint64_t min_expect_ackno = 1;
  uint64_t max_expect_ackno = 1;
  for ( auto [off, sz] : seq_size ) {
    test_1.execute( SegmentArrives {}.with_seqno( rx_isn + 1 + off ).with_data( d.substr( off, sz ) ) );
    if ( off + 1 == min_expect_ackno ) {
      min_expect_ackno = min_expect_ackno + sz;
    }
    max_expect_ackno = max_expect_ackno + sz;
    test_1.execute( ExpectAcknoBetween { rx_isn, off, min_expect_ackno, max_expect_ackno } );
  }

  test_1.execute( BytesPending { 0 } );
  test_1.execute( ReadAll { d } );
}

void do_test_2( const TCPConfig& cfg, default_random_engine& rd )
{
  const Wrap32 rx_isn( rd() );
  TCPReceiverTestHarness test_2 { "overlapping out-of-order segments", cfg.recv_capacity };
  test_2.execute( SegmentArrives {}.with_seqno( rx_isn ).with_syn() );
  vector<tuple<size_t, size_t>> seq_size;
  size_t datalen = 0;
  while ( datalen < cfg.recv_capacity ) {
    const size_t size = min( 1 + ( static_cast<size_t>( rd() ) % ( TCPConfig::MAX_PAYLOAD_SIZE - 1 ) ),
                             cfg.recv_capacity - datalen );
    const size_t rem = TCPConfig::MAX_PAYLOAD_SIZE - size;
    size_t offs = 0;
    if ( rem == 0 ) {
      offs = 0;
    } else if ( rem == 1 ) {
      offs = min( static_cast<size_t>( 1 ), datalen );
    } else {
      offs = min( min( datalen, rem ), 1 + ( static_cast<size_t>( rd() ) % ( rem - 1 ) ) );
    }

    if ( size + offs > TCPConfig::MAX_PAYLOAD_SIZE ) {
      throw runtime_error( "test 2 internal error: bad payload size" );
    }
    seq_size.emplace_back( datalen - offs, size + offs );
    datalen += size;
  }
  if ( datalen > cfg.recv_capacity ) {
    throw runtime_error( "test 2 internal error: bad offset sequence" );
  }
  shuffle( seq_size.begin(), seq_size.end(), rd );

  string d( datalen, 0 );
  generate( d.begin(), d.end(), [&] { return rd(); } );

  uint64_t min_expect_ackno = 1;
  uint64_t max_expect_ackno = 1;
  for ( auto [off, sz] : seq_size ) {
    test_2.execute( SegmentArrives {}.with_seqno( rx_isn + 1 + off ).with_data( d.substr( off, sz ) ) );
    if ( off + 1 <= min_expect_ackno && off + sz + 1 > min_expect_ackno ) {
      min_expect_ackno = sz + off;
    }
    max_expect_ackno = max_expect_ackno + sz; // really loose because of overlap
    test_2.execute( ExpectAcknoBetween { rx_isn, off, min_expect_ackno, max_expect_ackno } );
  }

  test_2.execute( BytesPending { 0 } );
  test_2.execute( ReadAll { d } );
}

int main()
{
  try {
    TCPConfig cfg {};
    cfg.recv_capacity = 65000;
    auto rd = get_random_engine();

    // non-overlapping out-of-order segments
    for ( unsigned rep_no = 0; rep_no < NREPS; ++rep_no ) {
      do_test_1( cfg, rd );
    }

    // overlapping out-of-order segments
    for ( unsigned rep_no = 0; rep_no < NREPS; ++rep_no ) {
      do_test_2( cfg, rd );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
