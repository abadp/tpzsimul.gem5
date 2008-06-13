/*
 * Copyright (c) 2004, 2005
 * The Regents of The University of Michigan
 * All Rights Reserved
 *
 * This code is part of the M5 simulator.
 *
 * Permission is granted to use, copy, create derivative works and
 * redistribute this software and such derivative works for any
 * purpose, so long as the copyright notice above, this grant of
 * permission, and the disclaimer below appear in all copies made; and
 * so long as the name of The University of Michigan is not used in
 * any advertising or publicity pertaining to the use or distribution
 * of this software without specific, written prior authorization.
 *
 * THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. THE REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE
 * LIABLE FOR ANY DAMAGES, INCLUDING DIRECT, SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN
 * IF IT HAS BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGES.
 *
 * Authors: Ali G. Saidi
 *          Andrew L. Schultz
 *          Miguel J. Serrano
 */

#ifndef __DEV_8254_HH__
#define __DEV_8254_HH__

#include "base/bitunion.hh"
#include "sim/eventq.hh"
#include "sim/host.hh"
#include "sim/serialize.hh"

#include <string>
#include <iostream>

/** Programmable Interval Timer (Intel 8254) */
class Intel8254Timer
{
    BitUnion8(CtrlReg)
        Bitfield<7, 6> sel;
        Bitfield<5, 4> rw;
        Bitfield<3, 1> mode;
        Bitfield<0> bcd;
    EndBitUnion(CtrlReg)

    enum SelectVal {
        SelectCounter0,
        SelectCounter1,
        SelectCounter2,
        ReadBackCommand
    };

    enum ReadWriteVal {
        LatchCommand,
        LsbOnly,
        MsbOnly,
        TwoPhase
    };

    enum ModeVal {
        InitTc,
        OneShot,
        RateGen,
        SquareWave,
        SoftwareStrobe,
        HardwareStrobe
    };

    /** Counter element for PIT */
    class Counter
    {
        /** Event for counter interrupt */
        class CounterEvent : public Event
        {
          private:
            /** Pointer back to Counter */
            Counter* counter;
            Tick interval;

          public:
            CounterEvent(Counter*);

            /** Event process */
            virtual void process();

            /** Event description */
            virtual const char *description() const;

            friend class Counter;

            void setTo(int clocks);
        };

      private:
        std::string _name;
        const std::string &name() const { return _name; }

        CounterEvent event;

        /** Current count value */
        uint16_t count;

        /** Latched count */
        uint16_t latched_count;

        /** Interrupt period */
        uint16_t period;

        /** Current mode of operation */
        uint8_t mode;

        /** Output goes high when the counter reaches zero */
        bool output_high;

        /** State of the count latch */
        bool latch_on;

        /** Set of values for read_byte and write_byte */
        enum {LSB, MSB};

        /** Determine which byte of a 16-bit count value to read/write */
        uint8_t read_byte, write_byte;

      public:
        Counter(const std::string &name);

        /** Latch the current count (if one is not already latched) */
        void latchCount();

        /** Set the read/write mode */
        void setRW(int rw_val);

        /** Set operational mode */
        void setMode(int mode_val);

        /** Set count encoding */
        void setBCD(int bcd_val);

        /** Read a count byte */
        uint8_t read();

        /** Write a count byte */
        void write(const uint8_t data);

        /** Is the output high? */
        bool outputHigh();

        /**
         * Serialize this object to the given output stream.
         * @param base The base name of the counter object.
         * @param os   The stream to serialize to.
         */
        void serialize(const std::string &base, std::ostream &os);

        /**
         * Reconstruct the state of this object from a checkpoint.
         * @param base The base name of the counter object.
         * @param cp The checkpoint use.
         * @param section The section name of this object
         */
        void unserialize(const std::string &base, Checkpoint *cp,
                         const std::string &section);
    };

  private:
    std::string _name;
    const std::string &name() const { return _name; }

    /** PIT has three seperate counters */
    Counter *counter[3];

  public:
    /** Public way to access individual counters (avoid array accesses) */
    Counter counter0;
    Counter counter1;
    Counter counter2;

    Intel8254Timer(const std::string &name);

    /** Write control word */
    void writeControl(const CtrlReg data);

    /**
     * Serialize this object to the given output stream.
     * @param base The base name of the counter object.
     * @param os The stream to serialize to.
     */
    void serialize(const std::string &base, std::ostream &os);

    /**
     * Reconstruct the state of this object from a checkpoint.
     * @param base The base name of the counter object.
     * @param cp The checkpoint use.
     * @param section The section name of this object
     */
    void unserialize(const std::string &base, Checkpoint *cp,
                     const std::string &section);
};

#endif // __DEV_8254_HH__
