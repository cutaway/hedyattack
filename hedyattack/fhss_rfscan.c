
void rf_scan(void)
{
    // setup RF
    // preconfigure channel information
    // loop 10 times to get RSSI norms
    // * loop through channels
    //   * change channel
    //   * take RSSI value
    //   * store MIN RSSI value
    //
    // loop 10 times to get channel spacing
    // * loop through channels at X spacing
    //   * change channel
    //   * take RSSI value
    //   * store MAX RSSI value
    // * take standard deviation to find peaks
    // * identify peaks within a certain differential to minimize odd RF sources 
    //
    // loop until dongle_state != RFSCAN
    // * loop through channels
    //   * change channel
    //   * take RSSI value
    //   * if RSSI is > NORM_RSSI + rssi_threshhold:
    //     * store channel/RSSI value in chan_hop_array
    //     * if chan_hop_array is full:
    //       * set array for USB TX  (send results back over USB)
    // * flashLED after each loop through
    //
    //
    // RX interrupt handler must be able to (protocol):
    // * change channel base and width
    // * change state (general, not application based)
    //
    // USB interface needs a debug channel
    // All TX must start with an APP number (or just flags?)
}
