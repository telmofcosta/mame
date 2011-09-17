/***************************************************************************

  Data East Pinball BSMT2000 sound board
 
  used for System 3 and Whitestar pinball games and Tattoo Assassins video

***************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "audio/decobsmt.h"

#define	M6809_TAG	"soundcpu"
#define BSMT_TAG	"bsmt"

/*
    Overriding the child device's memory map fails when done from a parent device rather than the base driver.
    Uncomment this out to observe.
*/
//#define USE_OVERRIDE_MAP

static ADDRESS_MAP_START( decobsmt_map, AS_PROGRAM, 8, decobsmt_device )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x2001) AM_WRITE(bsmt_reset_w)
	AM_RANGE(0x2002, 0x2003) AM_READ(bsmt_comms_r)
	AM_RANGE(0x2006, 0x2007) AM_READ(bsmt_status_r)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(bsmt0_w)
	AM_RANGE(0xa000, 0xa0ff) AM_WRITE(bsmt1_w)
	AM_RANGE(0x2000, 0xffff) AM_ROM AM_REGION(":soundcpu", 0x2000)
ADDRESS_MAP_END

#ifdef USE_OVERRIDE_MAP
static ADDRESS_MAP_START( bsmt_map, AS_0, 8, decobsmt_device )
	AM_RANGE(0x000000, 0xffffff) AM_ROM AM_REGION(":bsmt", 0)
ADDRESS_MAP_END
#endif

ROM_START( decobsmt )
	ROM_REGION(0x1000000, BSMT_TAG, ROMREGION_ERASE00)
ROM_END

static INTERRUPT_GEN( decobsmt_firq_interrupt )
{
	device_set_input_line(device, M6809_FIRQ_LINE, HOLD_LINE);
}                                              

static void bsmt_ready_callback(bsmt2000_device &device)
{
	decobsmt_device *decobsmt = device.machine().device<decobsmt_device>(DECOBSMT_TAG);
	device_set_input_line(decobsmt->m_ourcpu, M6809_IRQ_LINE, ASSERT_LINE); /* BSMT is ready */
}

MACHINE_CONFIG_FRAGMENT( decobsmt )
    MCFG_CPU_ADD(M6809_TAG, M6809, (3579580/2))
	MCFG_CPU_PROGRAM_MAP(decobsmt_map)
	MCFG_CPU_PERIODIC_INT(decobsmt_firq_interrupt, 489) /* Fixed FIRQ of 489Hz as measured on real (pinball) machine */

    MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_BSMT2000_ADD("bsmt", 24000000)
#ifdef USE_OVERRIDE_MAP
	MCFG_DEVICE_ADDRESS_MAP(AS_0, bsmt_map)
#endif
	MCFG_BSMT2000_READY_CALLBACK(bsmt_ready_callback)
	MCFG_SOUND_ROUTE(0, "lspeaker", 2.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 2.0)
MACHINE_CONFIG_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type DECOBSMT = &device_creator<decobsmt_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor decobsmt_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( decobsmt );
}

const rom_entry *decobsmt_device::device_rom_region() const
{
	return ROM_NAME( decobsmt );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  decobsmt_device - constructor
//-------------------------------------------------

decobsmt_device::decobsmt_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, DECOBSMT, "Data East/Sega/Stern BSMT2000 Sound Board", tag, owner, clock),
	m_ourcpu(*this, M6809_TAG),
	m_bsmt(*this, BSMT_TAG)
{
	m_shortname = "decobsmt";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void decobsmt_device::device_start()
{
#ifndef USE_OVERRIDE_MAP
    UINT8 *romsrc = machine().region("bsmt")->base();
	astring tempstring;
    UINT8 *romdst = machine().region(subtag(tempstring, "bsmt"))->base(); 

    memcpy(romdst, romsrc, 0x1000000);
#endif
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void decobsmt_device::device_reset()
{
	m_bsmt_latch = 0;
	m_bsmt_reset = 0;
	m_bsmt_comms = 0;
}

WRITE8_MEMBER(decobsmt_device::bsmt_reset_w)
{
	UINT8 diff = data ^ m_bsmt_reset;
	m_bsmt_reset = data;
	if ((diff & 0x80) && !(data & 0x80))
		m_bsmt->reset();
}

WRITE8_MEMBER(decobsmt_device::bsmt0_w)
{
	m_bsmt_latch = data;
}

WRITE8_MEMBER(decobsmt_device::bsmt1_w)
{
	m_bsmt->write_reg(offset ^ 0xff);
	m_bsmt->write_data((m_bsmt_latch << 8) | data);
	device_set_input_line(m_ourcpu, M6809_IRQ_LINE, CLEAR_LINE); /* BSMT is not ready */
}

READ8_MEMBER(decobsmt_device::bsmt_status_r)
{
	return m_bsmt->read_status() << 7;
}

READ8_MEMBER(decobsmt_device::bsmt_comms_r)
{
	return m_bsmt_comms;
}

WRITE8_MEMBER(decobsmt_device::bsmt_comms_w)
{
	m_bsmt_comms = data;
}

WRITE_LINE_MEMBER(decobsmt_device::bsmt_reset_line)
{
    device_set_input_line(m_ourcpu, INPUT_LINE_RESET, state);
}

