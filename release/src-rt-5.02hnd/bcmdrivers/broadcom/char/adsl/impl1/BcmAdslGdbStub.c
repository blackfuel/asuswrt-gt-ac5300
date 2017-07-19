/*
 *  arch/mips/kernel/gdb-stub.c
 *
 *  Originally written by Glenn Engel, Lake Stevens Instrument Division
 *
 *  Contributed by HP Systems
 *
 *  Modified for SPARC by Stu Grossman, Cygnus Support.
 *
 *  Modified for Linux/MIPS (and MIPS in general) by Andreas Busse
 *  Send complaints, suggestions etc. to <andy@waldorf-gmbh.de>
 *
 *  Copyright (C) 1995 Andreas Busse
 *
 *  Copyright (C) 2003 MontaVista Software Inc.
 *  Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 */

/*
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a BREAK instruction.
 *
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 *    bBB..BB	    Set baud rate to BB..BB		   OK or BNN, then sets
 *							   baud rate
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 *
 *  ==============
 *  MORE EXAMPLES:
 *  ==============
 *
 *  For reference -- the following are the steps that one
 *  company took (RidgeRun Inc) to get remote gdb debugging
 *  going. In this scenario the host machine was a PC and the
 *  target platform was a Galileo EVB64120A MIPS evaluation
 *  board.
 *
 *  Step 1:
 *  First download gdb-5.0.tar.gz from the internet.
 *  and then build/install the package.
 *
 *  Example:
 *    $ tar zxf gdb-5.0.tar.gz
 *    $ cd gdb-5.0
 *    $ ./configure --target=mips-linux-elf
 *    $ make
 *    $ install
 *    $ which mips-linux-elf-gdb
 *    /usr/local/bin/mips-linux-elf-gdb
 *
 *  Step 2:
 *  Configure linux for remote debugging and build it.
 *
 *  Example:
 *    $ cd ~/linux
 *    $ make menuconfig <go to "Kernel Hacking" and turn on remote debugging>
 *    $ make
 *
 *  Step 3:
 *  Download the kernel to the remote target and start
 *  the kernel running. It will promptly halt and wait
 *  for the host gdb session to connect. It does this
 *  since the "Kernel Hacking" option has defined
 *  CONFIG_KGDB which in turn enables your calls
 *  to:
 *     set_debug_traps();
 *     breakpoint();
 *
 *  Step 4:
 *  Start the gdb session on the host.
 *
 *  Example:
 *    $ mips-linux-elf-gdb vmlinux
 *    (gdb) set remotebaud 115200
 *    (gdb) target remote /dev/ttyS1
 *    ...at this point you are connected to
 *       the remote target and can use gdb
 *       in the normal fasion. Setting
 *       breakpoints, single stepping,
 *       printing variables, etc.
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#include <generated/autoconf.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/reboot.h>

#if !defined(CONFIG_BCM963138) && ! defined(CONFIG_BCM963148)
#include <asm/asm.h>
#include <asm/cacheflush.h>
#include <asm/mipsregs.h>
#include <asm/pgtable.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
// file no longer exists...
#else
#include <asm/system.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
// file no longer exists...
#else
#include <asm/gdb-stub.h>
#endif
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define __MIPSEL__
#include <arch/mips/include/asm/inst.h>
#else
#include <asm/inst.h>
#endif

#include <bcmtypes.h>
#include "AdslCoreMap.h"

#include "softdsl/SoftDsl.h"
#include "BcmAdslDiag.h"

char gdb_attached = 0;

extern int GdbWriteData(char *buf, int count);

#define MAX_COMM_BUFFER_LEN  1400

typedef struct {
  unsigned char *buf;
  int  len;
} comm_buffer;

static comm_buffer tx_buffer, rx_buffer;
static unsigned char outbuf[MAX_COMM_BUFFER_LEN];

static int phy_in_trap = 0;

enum WAITING_STATE {
  WAIT_MSG,
  WAIT_ACK
};

static enum WAITING_STATE protocol_state = WAIT_MSG;

/*
  extern void trap_low(void);
*/

/*
 * breakpoint and test functions
 */
extern void breakpoint(void);
extern void breakinst(void);
extern void async_breakpoint(void);
extern void async_breakinst(void);
extern void adel(void);

/*
 * local prototypes
 */

static void getpacket(char *buffer);
static void putpacket(char *buffer);
static int computeSignal(int tt);
static int hex(unsigned char ch);
static int hexToInt(char **ptr, int *intValue);
static int hexToLong(char **ptr, long *longValue);
static unsigned char *mem2hex(char *mem, char *buf, int count, int may_fault);

/* PHY register enumeration */
#define HI_REG_OFFSET       32
#define LO_REG_OFFSET       33
#define STATUS_REG_OFFSET   0
#define CAUSE_REG_OFFSET    1
#define EPC_REG_OFFSET      2

/* GDB frontend definitions */
#define REG_EPC 37
#define REG_FP  72
#define REG_SP  29

/* Communication with PHY MIPS */
enum PHY_STUB_CMD {
  PHY_STUB_CONTINUE,
  PHY_STUB_TRAP,
  PHY_STUB_ACK,
  PHY_STUB_READ_REG,
  PHY_STUB_READ_CP0_REG,
  PHY_STUB_WRITE_REG,
  PHY_STUB_WRITE_CP0_REG,
  PHY_STUB_READ_MEM,
  PHY_STUB_WRITE_MEM,
  PHY_STUB_STOP,
  PHY_STUB_NOP
};

struct gdbMboxType {
  char	gdbOn;
  char	gdbCmd;
  char	reserved[2];
  long	param0;
  long	param1;
  long	param2;
};

static struct gdbMboxType volatile *gdb_mbox = (struct gdbMboxType *)(HOST_LMEM_BASE + 0x3f0);

/*
 * spin lock to make BcmAdslCoreGdbTask reentrant
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
static spinlock_t gdb_task_lock;
#else
/* Not sure why this is initialized here.. likely can be removed... */
static spinlock_t gdb_task_lock = SPIN_LOCK_UNLOCKED;
#endif



/* Attach to running PHY code flag */
static int attach_mode = 0; 

/* Potmorten mode. PHY does not respond  */
static int postmorten_mode = 0;

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound buffers
 * at least NUMREGBYTES*2 are needed for register packets
 */
#define BUFMAX 2048

static char input_buffer[BUFMAX];
static char output_buffer[BUFMAX];
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
static int initialized;	/* !0 means we've been initialized */
#endif
static const char hexchars[]="0123456789abcdef";

/* write a single character */
int 
putDebugChar(char c)
{
  if (tx_buffer.len < MAX_COMM_BUFFER_LEN) {
    tx_buffer.buf[tx_buffer.len++] = c;
    //printk("putDebugChar::c=%c len=%d\n", c, tx_buffer.len);
    return tx_buffer.len;
  }
  return 0;
}
    
/* read and return a single char */
char 
getDebugChar(void)
{
  //printk("***getDebugChar::rx_buffer.len=%d char = %c\n", rx_buffer.len, rx_buffer.buf[rx_buffer.len]);
  if (rx_buffer.len <  MAX_COMM_BUFFER_LEN)
    return rx_buffer.buf[rx_buffer.len++];
  return 0;
}
    
/*
 * Convert ch from a hex digit to an int
 */
static int 
hex(unsigned char ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch-'a'+10;
  if (ch >= '0' && ch <= '9')
    return ch-'0';
  if (ch >= 'A' && ch <= 'F')
    return ch-'A'+10;
  return -1;
}

/*
 * scan for the sequence $<data>#<checksum>
 */
static void 
getpacket(char *buffer)
{
  unsigned char checksum;
  unsigned char xmitcsum;
  int i;
  int count;
  unsigned char ch;

  /*
   * Return if not a start character.
   */
  if ((ch = getDebugChar() & 0x7f) != '$') {
    protocol_state = WAIT_MSG;
    return;
  }

  //printk("***getpacket::'$'\n");
  checksum = 0;
  xmitcsum = -1;
  count = 0;

  /*
   * now, read until a # or end of buffer is found
   */
  while (count < BUFMAX) {
    ch = getDebugChar();
    if (ch == '#')
      break;
    checksum = checksum + ch;
    buffer[count] = ch;
    count = count + 1;
  }

  if (count >= BUFMAX) {
    protocol_state = WAIT_MSG;
    return;
  }

  buffer[count] = 0;

  if (ch == '#') {
    xmitcsum = hex(getDebugChar() & 0x7f) << 4;
    xmitcsum |= hex(getDebugChar() & 0x7f);

    //printk("getpacket::checksum=%x xmitcsum=%x\n", checksum, xmitcsum);
    tx_buffer.len = 0;
    if (checksum != xmitcsum)
      putDebugChar('-');	/* failed checksum */
    else {
      putDebugChar('+'); /* successful transfer */

      /*
       * if a sequence char is present,
       * reply the sequence ID
       */
      if (buffer[2] == ':') {
	putDebugChar(buffer[0]);
	putDebugChar(buffer[1]);

	/*
	 * remove sequence chars from buffer
	 */
	count = strlen(buffer);
	for (i=3; i <= count; i++)
	  buffer[i-3] = buffer[i];
      }
    }
    protocol_state = WAIT_MSG;
    GdbWriteData(tx_buffer.buf, tx_buffer.len);
  }
}

/*
 * send the packet in buffer.
 */
static void 
putpacket(char *buffer)
{
  unsigned char checksum;
  int count;
  unsigned char ch;

  /*
   * $<packet info>#<checksum>.
   */

  tx_buffer.len = 0;
  putDebugChar('$');
  checksum = 0;
  count = 0;

  while ((ch = buffer[count]) != 0) {
    if (!(putDebugChar(ch))) {
      protocol_state = WAIT_MSG;
      return;
    }
    checksum += ch;
    count += 1;
  }

  putDebugChar('#');
  putDebugChar(hexchars[checksum >> 4]);
  putDebugChar(hexchars[checksum & 0xf]);

  protocol_state = WAIT_ACK; 
  GdbWriteData(tx_buffer.buf, tx_buffer.len);  
}

static int
phy_write_mem(char *addr, char* val, int size)
{
  int i;

  for (i = 0; i < size; i++) {
    gdb_mbox->param0 = ADSL_ENDIAN_CONV_LONG(((long)addr & 0xbfffffff));
    addr++;
    gdb_mbox->param1 = ADSL_ENDIAN_CONV_LONG(val[i]);
    gdb_mbox->gdbCmd = PHY_STUB_WRITE_MEM;
    while (gdb_mbox->gdbCmd == PHY_STUB_WRITE_MEM);
    if (gdb_mbox->gdbCmd != PHY_STUB_ACK)
      return 0;
  }
  return 1;
}

static int phy_write_long(char *addr, long val)
{
  val = ADSL_ENDIAN_CONV_LONG(val); 
  return phy_write_mem(addr, (char *)&val, sizeof(long));
}

/* Read PHY GP registers - returns pointer to GDB exception stack */
static char *
phy_read_regs(int convAddrToHost)
{
  unsigned long  phyAddr;

  gdb_mbox->gdbCmd = PHY_STUB_READ_REG;
  while (gdb_mbox->gdbCmd != PHY_STUB_ACK);
  phyAddr = ADSL_ENDIAN_CONV_LONG(gdb_mbox->param0);
  if (convAddrToHost)
	phyAddr = (unsigned long) (ADSL_ADDR_TO_HOST(phyAddr));
  return (char *) phyAddr;
}

/* Read some PHY CP0 regs */
static char *
phy_read_cp0_regs(int convAddrToHost)
{
  unsigned long  phyAddr;

  gdb_mbox->gdbCmd = PHY_STUB_READ_CP0_REG;
  while (gdb_mbox->gdbCmd != PHY_STUB_ACK);
  phyAddr = ADSL_ENDIAN_CONV_LONG(gdb_mbox->param0);
  if (convAddrToHost)
	phyAddr = (unsigned long) (ADSL_ADDR_TO_HOST(phyAddr));
  return (char *) phyAddr;
}

static void
phy_write_cp0_reg(int reg)
{
  gdb_mbox->param0 = ADSL_ENDIAN_CONV_LONG(reg);
  gdb_mbox->gdbCmd = PHY_STUB_WRITE_CP0_REG;
  while(gdb_mbox->gdbCmd != PHY_STUB_ACK);
}

static void
phy_send_interrupt(void)
{
  unsigned long volatile *pAdslEnum = (unsigned long *) XDSL_ENUM_BASE;
  printk("*** PHY interrupted\n");
  BcmAdslCoreDiagWriteStatusString(0, "*** PHY interrupted");
  gdb_mbox->gdbCmd = PHY_STUB_STOP;
  pAdslEnum[ADSL_HOSTMESSAGE] = 1;
}

/*
 * Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 * may_fault is non-zero if we are reading from arbitrary memory, but is currently
 * not used.
 */
static unsigned char *
mem2hex(char *mem, char *buf, int count, int may_fault)
{
  unsigned char ch;

  while (count-- > 0) {
    ch = *mem++;
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch & 0xf];
  }

  *buf = 0;
  return buf;
}

static unsigned char *long2hex(long val, char *buf)
{
  val = ADSL_ENDIAN_CONV_LONG(val); 
  return mem2hex((char *)&val, buf, sizeof(long), 0);
}

/*
 * convert the hex array pointed to by buf into binary to be placed in mem
 * return a pointer to the character AFTER the last byte written
 * may_fault is non-zero if we are reading from arbitrary memory, but is currently
 * not used.
 */
static char *
hex2mem(char *buf, char *mem, int count, int binary, int may_fault)
{
  int i;
  unsigned char ch;

  for (i=0; i<count; i++) {
    if (binary) {
      ch = *buf++;
      if (ch == 0x7d)
	ch = 0x20 ^ *buf++;
    }
    else {
      ch = hex(*buf++) << 4;
      ch |= hex(*buf++);
    }

    if (!phy_write_mem(mem++, &ch, 1))
      return 0;
  }
  return mem;
}

/*
 * This table contains the mapping between SPARC hardware trap types, and
 * signals, which are primarily what GDB understands.  It also indicates
 * which hardware traps we need to commandeer when initializing the stub.
 */
static struct hard_trap_info {
  unsigned char tt;		/* Trap type code for MIPS R3xxx and R4xxx */
  unsigned char signo;		/* Signal that we map this trap into */
} hard_trap_info[] = {
  { 0, SIGINT },                        /* user's  interrupt */
  { 6, SIGBUS },			/* instruction bus error */
  { 7, SIGBUS },			/* data bus error */
  { 9, SIGTRAP },			/* break */
  { 10, SIGILL },			/* reserved instruction */
  /*	{ 11, SIGILL },		*/	/* CPU unusable */
  { 12, SIGFPE },			/* overflow */
  { 13, SIGTRAP },		/* trap */
  { 14, SIGSEGV },		/* virtual instruction cache coherency */
  { 15, SIGFPE },			/* floating point exception */
  { 23, SIGSEGV },		/* watch */
  { 31, SIGSEGV },		/* virtual data cache coherency */
  { 0xff, 0}				/* Must be last */
};

/* Save the normal trap handlers for user-mode traps. */
void *saved_vectors[32];


/*
 * Convert the MIPS hardware trap type code to a Unix signal number.
 */
static int 
computeSignal(int tt)
{
  struct hard_trap_info *ht;

  for (ht = hard_trap_info; (ht->tt != 0xff); ht++)
    if (ht->tt == tt)
      return ht->signo;

  return SIGHUP;		/* default for things we don't know about */
}

/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */
static int 
hexToInt(char **ptr, int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr) {
    hexValue = hex(**ptr);
    if (hexValue < 0)
      break;

    *intValue = (*intValue << 4) | hexValue;
    numChars ++;

    (*ptr)++;
  }

  return (numChars);
}

static int 
hexToLong(char **ptr, long *longValue)
{
  int numChars = 0;
  int hexValue;

  *longValue = 0;

  while (**ptr) {
    hexValue = hex(**ptr);
    if (hexValue < 0)
      break;

    *longValue = (*longValue << 4) | hexValue;
    numChars ++;

    (*ptr)++;
  }

  return numChars;
}

/*
 * We single-step by setting breakpoints. When an exception
 * is handled, we need to restore the instructions hoisted
 * when the breakpoints were set.
 *
 * This is where we save the original instructions.
 */
static struct gdb_bp_save {
  unsigned long addr;
  unsigned int val;
} step_bp[2];

const int BP=0x0000000d;  /* break opcode */

/*
 * Set breakpoint instructions for single stepping.
 */
static void 
single_step(void)
{
  union mips_instruction insn;
  unsigned long targ, statReg;
  int is_branch, is_cond, i;
  long *gp_regs = (long *)phy_read_regs(1);
  long *cp0_regs = (long *)phy_read_cp0_regs(1);
  unsigned long cp0_epc = ADSL_ENDIAN_CONV_LONG(cp0_regs[EPC_REG_OFFSET]);

  //printk("single_step::cp0_epc=0x%lx\n", cp0_epc);
  //printk("single_step::PhyStack=0x%08lx\n", (long)gp_regs);

  targ = cp0_epc;
  insn.word = ADSL_ENDIAN_CONV_LONG(*(unsigned int *)(ADSL_ADDR_TO_HOST(targ)));
  //printk("single_step::inst=0x%x\n", insn.word);
  is_branch = is_cond = 0;

  switch (insn.i_format.opcode) {
    /*
     * jr and jalr are in r_format format.
     */
  case spec_op:
    switch (insn.r_format.func) {
    case jalr_op:
    case jr_op:
      //targ = *(&regs->reg0 + insn.r_format.rs);
      targ = ADSL_ENDIAN_CONV_LONG(*(unsigned long *)(ADSL_ADDR_TO_HOST(gp_regs + insn.r_format.rs - 1)));
      is_branch = 1;
      break;
    }
    break;

    /*
     * This group contains:
     * bltz_op, bgez_op, bltzl_op, bgezl_op,
     * bltzal_op, bgezal_op, bltzall_op, bgezall_op.
     */
  case bcond_op:
    is_branch = is_cond = 1;
    targ += 4 + (insn.i_format.simmediate << 2);
    break;

    /*
     * These are unconditional and in j_format.
     */
  case jal_op:
  case j_op:
    is_branch = 1;
    targ += 4;
    targ >>= 28;
    targ <<= 28;
    targ |= (insn.j_format.target << 2);
    break;

    /*
     * These are conditional.
     */
  case beq_op:
  case beql_op:
  case bne_op:
  case bnel_op:
  case blez_op:
  case blezl_op:
  case bgtz_op:
  case bgtzl_op:
  case cop0_op:
  case cop1_op:
  case cop2_op:
  case cop1x_op:
    is_branch = is_cond = 1;
    targ += 4 + (insn.i_format.simmediate << 2);
    break;
  }

  if (is_branch) {
    i = 0;
    if (is_cond && targ != (cp0_epc + 8)) {
      step_bp[i].addr = cp0_epc + 8;
      step_bp[i++].val = ADSL_ENDIAN_CONV_LONG(*(unsigned *)(ADSL_ADDR_TO_HOST(cp0_epc + 8)));
      //*(unsigned *)(regs->cp0_epc + 8) = BP;
      phy_write_long((char *)(cp0_epc + 8), BP);
    }
	//printk("single_step: branch: i=%d targ=0x%08lX\n", i, targ);
    step_bp[i].addr = targ;
    step_bp[i].val  = ADSL_ENDIAN_CONV_LONG(*(unsigned *)(ADSL_ADDR_TO_HOST(targ)));
    //*(unsigned *)targ = BP;
    phy_write_long((char *)targ, BP);
  } else {
    step_bp[0].addr = cp0_epc + 4;
    step_bp[0].val  = ADSL_ENDIAN_CONV_LONG(*(unsigned *)(ADSL_ADDR_TO_HOST(cp0_epc + 4)));
    //printk("single_step::0x%lx: 0x%lx\n", step_bp[0].addr, step_bp[0].val);
    //*(unsigned *)(regs->cp0_epc + 4) = BP;
    phy_write_long((char *)(cp0_epc + 4), BP);
  }

  statReg = ADSL_ENDIAN_CONV_LONG(cp0_regs[STATUS_REG_OFFSET]);
  statReg = (statReg >> 1) << 1;
  cp0_regs[STATUS_REG_OFFSET] = ADSL_ENDIAN_CONV_LONG(statReg);
  phy_write_cp0_reg(STATUS_REG_OFFSET);
  //printk("single_step:statReg1=0x%08lX 0x%08lX\n", statReg, cp0_regs[STATUS_REG_OFFSET]);
}

void
send_trap_packet(int first_break)
{
  int trap;
  int sigval;
  char *ptr;
  long *gp_regs = (long *)phy_read_regs(1);
  long *cp0_regs = (long *)phy_read_cp0_regs(1);

  long cp0_cause = ADSL_ENDIAN_CONV_LONG(cp0_regs[CAUSE_REG_OFFSET]);
  long cp0_epc = ADSL_ENDIAN_CONV_LONG(cp0_regs[EPC_REG_OFFSET]);
  long reg30 = ADSL_ENDIAN_CONV_LONG(gp_regs[29]);
  long reg29 = ADSL_ENDIAN_CONV_LONG(gp_regs[28]);

  /*
   * If we stopped at first (dummy) breakpoint, increment the PC
   */
  if (first_break == 1 && phy_in_trap == 0) {
    printk("Waiting for remote gdb ...\n");
    return;
  }

  trap = (cp0_cause & 0x7c) >> 2;
#if 0
  if (trap == 9 && first_break == 1) {
    /* write back cp0_epc + 4 */
    cp0_epc += 4;
    phy_write_cp0_reg(EPC_REG_OFFSET, cp0_epc);
  }
#endif

  /*
   * If we were single_stepping, restore the opcodes hoisted
   * for the breakpoint[s].
   */
  if (step_bp[0].addr) {
	unsigned long statReg;

    //*(unsigned *)step_bp[0].addr = step_bp[0].val;
    phy_write_long((char *)step_bp[0].addr, step_bp[0].val); 
    step_bp[0].addr = 0;

    if (step_bp[1].addr) {
      //*(unsigned *)step_bp[1].addr = step_bp[1].val;
     phy_write_long((char *)step_bp[1].addr, step_bp[1].val);
     step_bp[1].addr = 0;
    }
    statReg = ADSL_ENDIAN_CONV_LONG(cp0_regs[STATUS_REG_OFFSET]);
    statReg = statReg | 1;
    cp0_regs[STATUS_REG_OFFSET] = ADSL_ENDIAN_CONV_LONG(statReg);
    phy_write_cp0_reg(STATUS_REG_OFFSET);
    //printk("trap:statReg1=0x%08lX 0x%08lX\n", statReg, cp0_regs[STATUS_REG_OFFSET]);
  }

  //stack = (long *)regs->reg29;			/* stack ptr */
  sigval = computeSignal(trap);

  /*
   * reply to host that an exception has occurred
   */
  ptr = output_buffer;

  /*
   * Send trap type (converted to signal)
   */
  *ptr++ = 'T';
  *ptr++ = hexchars[sigval >> 4];
  *ptr++ = hexchars[sigval & 0xf];

  /*
   * Send Error PC
   */
  *ptr++ = hexchars[REG_EPC >> 4];
  *ptr++ = hexchars[REG_EPC & 0xf];
  *ptr++ = ':';
  //ptr = mem2hex((char *)&cp0_epc, ptr, sizeof(long), 0);
  ptr = long2hex(cp0_epc, ptr);
  *ptr++ = ';';

  /*
   * Send frame pointer
   */
  *ptr++ = hexchars[REG_FP >> 4];
  *ptr++ = hexchars[REG_FP & 0xf];
  *ptr++ = ':';
  //ptr = mem2hex((char *)&reg30, ptr, sizeof(long), 0);
  ptr = long2hex(reg30, ptr);
  *ptr++ = ';';

  /*
   * Send stack pointer
   */
  *ptr++ = hexchars[REG_SP >> 4];
  *ptr++ = hexchars[REG_SP & 0xf];
  *ptr++ = ':';
  //ptr = mem2hex((char *)&reg29, ptr, sizeof(long), 0);
  ptr = long2hex(reg29, ptr);
  *ptr++ = ';';

  *ptr++ = 0;
  //printk("***TRAP_PACKET::%s\n", output_buffer);
  putpacket(output_buffer);	/* send it off... */
  phy_in_trap = 0;
}

void
parse_protocol(void)
{
  char *ptr;
  long addr;
  int length;
  int bflag = 0;
  char *gp_regs;
  char *cp0_regs;
  int i = 0;

  output_buffer[0] = 0;
  getpacket(input_buffer);

  switch (input_buffer[0])
    {
    case '?':
      if (phy_in_trap) {
#if 0
	output_buffer[0] = 'S';
	output_buffer[1] = hexchars[9 >> 4];
	output_buffer[2] = hexchars[9 & 0xf];
	output_buffer[3] = 0;
#else
	send_trap_packet(0);
	goto finish_kgdb;
#endif
      }
      else if (attach_mode == 1) {
	attach_mode = 0;
	phy_send_interrupt(); 
	do {
	  if (i == 1000) {
	    printk("*** PHY does not respond. GDB in post-morten mode.\n");
	    BcmAdslCoreDiagWriteStatusString(0, "*** PHY does not respond. GDB in post-morten mode.");
	    postmorten_mode = 1;
	    output_buffer[0] = 'S';
	    output_buffer[1] = hexchars[9 >> 4];
	    output_buffer[2] = hexchars[9 & 0xf];
	    output_buffer[3] = 0;
	    break;
	  }
	  i++;
	} while (gdb_mbox->gdbCmd == PHY_STUB_STOP);
      }
      else {
	gdb_attached = 1;
	printk("*** GDB is connected. Load PHY code.\n"); 
	BcmAdslCoreDiagWriteStatusString(0, "*** GDB is connected. Load PHY code.");	
	goto finish_kgdb;
      }
      break;

      /*
       * Attach to runninig target, just interrupt target
       */
    case 'a':
      attach_mode = 1;
      gdb_mbox->gdbOn = 1;
      gdb_attached = 1;
      break;

      /*
       * Detach debugger; let CPU run
       */
    case 'D':
      gdb_mbox->gdbOn = 0;
      gdb_attached = 0;
      phy_in_trap = 0;
      postmorten_mode = 0;
      do {
	gdb_mbox->gdbCmd = PHY_STUB_CONTINUE;
      } while (gdb_mbox->gdbCmd != PHY_STUB_CONTINUE);
      strcpy(output_buffer,"OK");
      printk("*** GDB disconnected\n");
      BcmAdslCoreDiagWriteStatusString(0, "*** GDB disconnected");
      break;

    case 'd':
      /* toggle debug flag */
      break;

      /*
       * Return the value of the CPU registers
       */
    case 'g':
      {
	long reg_r0 = 0;
	if (postmorten_mode != 1) {
	
	gp_regs = (char *) phy_read_regs(1);
	//printk("***ReadReg=0x%08lx\n", (long)gp_regs);
	cp0_regs = (char *) phy_read_cp0_regs(1);
	ptr = output_buffer;      
	//ptr = mem2hex((char *)&reg_r0, ptr, sizeof(long), 0); /* r0, alvays 0 */
	ptr = long2hex(reg_r0, ptr);
	ptr = mem2hex(gp_regs, ptr, 31*sizeof(long), 0); /* r1...r31 */
	ptr = mem2hex(cp0_regs + STATUS_REG_OFFSET*sizeof(long), ptr, sizeof(long), 0); /* status */
	ptr = mem2hex(gp_regs + LO_REG_OFFSET*sizeof(long), ptr, sizeof(long), 0); /* lo */
	ptr = mem2hex(gp_regs + HI_REG_OFFSET*sizeof(long), ptr, sizeof(long), 0); /* hi */
	//ptr = mem2hex((char *)&reg_r0, ptr, sizeof(long), 0); /* FIXME (BadVaddr) */
	ptr = long2hex(reg_r0, ptr);
	ptr = mem2hex(cp0_regs + CAUSE_REG_OFFSET*sizeof(long), ptr, sizeof(long), 0); /* cause */
	ptr = mem2hex(cp0_regs + EPC_REG_OFFSET*sizeof(long), ptr, sizeof(long), 0); /* epc */
	}
	else {
	  ptr = output_buffer;
	  for (i = 0; i < 38*4; i++)
	    *ptr++ = 0x30;
	}
      }
      break;

      /*
       * set the value of the CPU registers - return OK
       */
    case 'G':
      {
	gp_regs = phy_read_regs(0);
	//printk("WriteRegs=0x%08lx\n", (long)gp_regs);
	cp0_regs = phy_read_cp0_regs(0);
	ptr = &input_buffer[1] + 2*sizeof(long); /* skip r0 */
	hex2mem(ptr, gp_regs, 31*sizeof(long), 0, 0); /* r1...r31 */
	ptr += 31*(2*sizeof(long));

	hex2mem(ptr, cp0_regs+STATUS_REG_OFFSET*sizeof(long), sizeof(long), 0, 0); /* status */
	phy_write_cp0_reg(STATUS_REG_OFFSET);
	ptr += 2*sizeof(long);

	hex2mem(ptr, gp_regs + LO_REG_OFFSET*sizeof(long), sizeof(long), 0, 0);
	ptr += 2*sizeof(long);

	hex2mem(ptr, gp_regs + HI_REG_OFFSET*sizeof(long), sizeof(long), 0, 0);
	ptr += 2*sizeof(long);

	ptr += 2*sizeof(long); /* FIXME: skip BadVaddr */ 

	hex2mem(ptr, cp0_regs+CAUSE_REG_OFFSET*sizeof(long), sizeof(long), 0, 0); /* cause */
	phy_write_cp0_reg(CAUSE_REG_OFFSET);
	ptr += 2*sizeof(long);

	hex2mem(ptr, cp0_regs+EPC_REG_OFFSET*sizeof(long), sizeof(long), 0, 0); /* epc */
	phy_write_cp0_reg(EPC_REG_OFFSET);
	ptr += 2*sizeof(long);

	strcpy(output_buffer,"OK");
      }
      break;

      /*
       * mAA..AA,LLLL  Read LLLL bytes at address AA..AA
       */
    case 'm':
      ptr = &input_buffer[1];
      if (hexToLong(&ptr, &addr) && *ptr++ == ',' && hexToInt(&ptr, &length)) {
#if 0
		if(addr & 0x80000000) {	/* Host addresses */
			if (mem2hex((char *)addr, output_buffer, length, 1))
				break;
		}
		else
#endif
		{
			long mapped_addr;
			addr &= ~0xC0000000;	/* Clear WriteBack Cache addresses */
			mapped_addr = (long) (ADSL_ADDR_TO_HOST(addr));
			if (mapped_addr != -1) {
				if (mem2hex((char *)mapped_addr, output_buffer, length, 1))
					break;
			}
		}
		strcpy (output_buffer, "E03");
	}
	else
		strcpy(output_buffer,"E01");
	break;

      /*
       * XAA..AA,LLLL: Write LLLL escaped binary bytes at address AA.AA
       */
    case 'X':
      bflag = 1;
      /* fall through */

      /*
       * MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK
       */
    case 'M':
      ptr = &input_buffer[1];
      // phy_write_memory();
      if (hexToLong(&ptr, &addr)
	  && *ptr++ == ','
	  && hexToInt(&ptr, &length)
	  && *ptr++ == ':') {
	if (hex2mem(ptr, (char *)addr, length, bflag, 1))
	  strcpy(output_buffer, "OK");
	else
	  strcpy(output_buffer, "E03");
      }
      else
	strcpy(output_buffer, "E02");
      break;

      /*
       * cAA..AA    Continue at address AA..AA(optional)
       */
    case 'c':
      /* try to read optional parameter, pc unchanged if no parm */
      cp0_regs = phy_read_cp0_regs(0);
      ptr = &input_buffer[1];
      if (hexToLong(&ptr, &addr)) {
        printk("cAA..AA: addr=0x%08lX\n", addr);
        phy_write_long(cp0_regs + EPC_REG_OFFSET*sizeof(long), addr);
        phy_write_cp0_reg(EPC_REG_OFFSET);
      }
      gdb_mbox->gdbCmd = PHY_STUB_CONTINUE;
      goto finish_kgdb;
      break;

      /*
       * kill the program; let us try to restart the machine
       * Reset the whole machine.
       */
    case 'k':
    case 'r':
      gdb_mbox->gdbOn = 0;
      gdb_attached = 0;
      phy_in_trap = 0;
      postmorten_mode = 0;
      gdb_mbox->gdbCmd = PHY_STUB_CONTINUE;      
      printk("*** GDB disconnected\n");
      BcmAdslCoreDiagWriteStatusString(0, "*** GDB disconnected");
      //machine_restart("kgdb restarts machine");
      break;

      /*
       * Step to next instruction
       */
    case 's':
      /*
       * There is no single step insn in the MIPS ISA, so we
       * use breakpoints and continue, instead.
       */
      single_step();
      gdb_mbox->gdbCmd = PHY_STUB_CONTINUE;
      goto finish_kgdb;
      /* NOTREACHED */
      break;

      /*
       * Set baud rate (bBB)
       * FIXME: Needs to be written
       */
    case 'b':
      {
#if 0
	int baudrate;
	extern void set_timer_3();

	ptr = &input_buffer[1];
	if (!hexToInt(&ptr, &baudrate))
	  {
	    strcpy(output_buffer,"B01");
	    break;
	  }

	/* Convert baud rate to uart clock divider */

	switch (baudrate)
	  {
	  case 38400:
	    baudrate = 16;
	    break;
	  case 19200:
	    baudrate = 33;
	    break;
	  case 9600:
	    baudrate = 65;
	    break;
	  default:
	    baudrate = 0;
	    strcpy(output_buffer,"B02");
	    goto x1;
	  }

	if (baudrate) {
	  putpacket("OK");	/* Ack before changing speed */
	  set_timer_3(baudrate); /* Set it */
	}
#endif
      }
      break;

    }			/* switch */

  /*
   * reply to the request
   */
  putpacket(output_buffer);

 finish_kgdb:
  return;
}

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
/*
 * This function will generate a breakpoint exception.  It is used at the
 * beginning of a program to sync up with a debugger and can be used
 * otherwise as a quick means to stop program execution and "break" into
 * the debugger.
 */
void breakpoint(void)
{
  if (!initialized)
    return;

  __asm__ __volatile__(
		       ".globl	breakinst\n\t" 
		       ".set\tnoreorder\n\t"
		       "nop\n"
		       "breakinst:\tbreak\n\t"
		       "nop\n\t"
		       ".set\treorder"
		       );
}

void adel(void)
{
  __asm__ __volatile__(
		       ".globl\tadel\n\t"
		       "lui\t$8,0x8000\n\t"
		       "lw\t$9,1($8)\n\t"
		       );
}
#endif

#if 0
/*
 * malloc is needed by gdb client in "call func()", even a private one
 * will make gdb happy
 */
static void *malloc(size_t size)
{
  return kmalloc(size, GFP_ATOMIC);
}

static void free(void *where)
{
  kfree(where);
}
#endif

char
isGdbOn(void)
{
  return gdb_mbox->gdbOn;
}

void
setGdbOn(void)
{
  //printk("+++setGdbOn=%d\n", gdb_attached);
  gdb_mbox->gdbOn = gdb_attached;
}

void 
BcmAdslCoreGdbTask(void)
{
  if (!spin_trylock(&gdb_task_lock))
    return;

  //printk("***BcmAdslCoreGdbTask::gdb_attached=%d gdbOn=%d phy_in_trap=%d\n", gdb_attached, gdb_mbox->gdbOn, phy_in_trap);
  if (gdb_mbox->gdbOn == 1) {
    if (gdb_mbox->gdbCmd == PHY_STUB_TRAP) {
      phy_in_trap = 1;
      send_trap_packet(ADSL_ENDIAN_CONV_LONG(gdb_mbox->param0));
    }
    else if ((gdb_mbox->gdbCmd == PHY_STUB_STOP) && (0 == postmorten_mode)) { /* Phy is dead, command PHY_STUB_STOP is not cleared */
      printk("*** PHY does not respond. GDB in post-morten mode.\n");
      BcmAdslCoreDiagWriteStatusString(0, "*** PHY does not respond. GDB in post-morten mode.");
      output_buffer[0] = 'S';
      output_buffer[1] = hexchars[9 >> 4];
      output_buffer[2] = hexchars[9 & 0xf];
      output_buffer[3] = 0;
      putpacket(output_buffer);
    }
  }
  spin_unlock(&gdb_task_lock);
}

void 
GdbStubInit(void)
{
  protocol_state = WAIT_MSG;
  tx_buffer.buf = outbuf;
  tx_buffer.len = 0;

  spin_lock_init(&gdb_task_lock);

  /* Let PHY stub know that gdb is attached */
  gdb_attached = 1;
  attach_mode = 0;
  postmorten_mode = 0;
}

/* Parse GDB packets */
void 
BcmAdslCoreGdbCmd(void *pCmd, int cmdLen)
{
  /* Initialize communication buffers */
  rx_buffer.buf = (unsigned char *)pCmd;
  rx_buffer.len = 0;

  if (rx_buffer.buf[0] == '\003') {
    phy_send_interrupt();
    return;
  }

  switch(protocol_state) {
  case WAIT_MSG:
    if (rx_buffer.buf[0] == '+')
      break;
    parse_protocol();
    break;
  case WAIT_ACK:
    if (rx_buffer.buf[0] == '+') { /* ACK */
      tx_buffer.buf = outbuf;
      tx_buffer.len = 0;
      protocol_state = WAIT_MSG;
    }
    else
      GdbWriteData(tx_buffer.buf, tx_buffer.len);
    break;
  default:
    break;
  }   
}
