
extern static xdata channel_info chan_table[NUM_CHANNELS];

//static xdata u8 chan_mins[NUM_CHANNELS];
extern static xdata u8 chan_str[NUM_CHANNELS];
extern static xdata u8 chan_maxs[NUM_CHANNELS];
extern static xdata channel_hop chan_hops[MAX_HOPS];
extern static xdata u16 chan_loop;

extern static xdata u8 mode;

// These are dynamic, therefore
// Make them configurable
extern static xdata u8 chan_num;
extern static xdata u32 chan_spacing;
extern static xdata u8 pos_chan_threshold;            // stores the "bar" for whether a channel has been hit.
extern static u8 ceiling, floor;
extern static u16 chan_hops_cnt;
extern static u16 chan_loop_cnt;
extern static u16 next_mode_cnt;
