#include <emmc.h>
#include <mbox.h>
#include <printf.h>


static void sd_power_off()
{
	/* Power off the SD card */
	u32 control0 = mmio_read(emmc_base + EMMC_CONTROL0);
	control0 &= ~(1 << 8);	// Set SD Bus Power bit off in Power Control Register
	mmio_write(emmc_base + EMMC_CONTROL0, control0);
}

static u32 sd_get_base_clock_hz()
{
    u32 base_clock;
#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_GENERIC
    capabilities_0 = mmio_read(emmc_base + EMMC_CAPABILITIES_0);
    base_clock = ((capabilities_0 >> 8) & 0xff) * 1000000;
#elif SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
	uintptr_t mb_addr = 0x00007000;		// 0x7000 in L2 cache coherent mode
	volatile u32 *mailbuffer = (u32 *)mb_addr;

	/* Get the base clock rate */
	// set up the buffer
	mailbuffer[0] = 8 * 4;		// size of this message
	mailbuffer[1] = 0;			// this is a request

	// next comes the first tag
	mailbuffer[2] = 0x00030002;	// get clock rate tag
	mailbuffer[3] = 0x8;		// value buffer size
	mailbuffer[4] = 0x4;		// is a request, value length = 4
	mailbuffer[5] = 0x1;		// clock id + space to return clock id
	mailbuffer[6] = 0;			// space to return rate (in Hz)

	// closing tag
	mailbuffer[7] = 0;

	// send the message
	mbox_write(MBOX_PROP, mb_addr);

	// read the response
	mbox_read(MBOX_PROP);

	if(mailbuffer[1] != MBOX_SUCCESS)
	{
	    printf("EMMC: property mailbox did not return a valid response.\n");
	    return 0;
	}

	if(mailbuffer[5] != 0x1)
	{
	    printf("EMMC: property mailbox did not return a valid clock id.\n");
	    return 0;
	}

	base_clock = mailbuffer[6];
#else
    printf("EMMC: get_base_clock_hz() is not implemented for this "
           "architecture.\n");
    return 0;
#endif

#ifdef EMMC_DEBUG
    printf("EMMC: base clock rate is %i Hz\n", base_clock);
#endif
    return base_clock;
}

#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
static int bcm_2708_power_off()
{
	uintptr_t mb_addr = 0x40007000;		// 0x7000 in L2 cache coherent mode
	volatile u32 *mailbuffer = (u32 *)mb_addr;

	/* Power off the SD card */
	// set up the buffer
	mailbuffer[0] = 8 * 4;		// size of this message
	mailbuffer[1] = 0;			// this is a request

	// next comes the first tag
	mailbuffer[2] = 0x00028001;	// set power state tag
	mailbuffer[3] = 0x8;		// value buffer size
	mailbuffer[4] = 0x8;		// is a request, value length = 8
	mailbuffer[5] = 0x0;		// device id and device id also returned here
	mailbuffer[6] = 0x2;		// set power off, wait for stable and returns state

	// closing tag
	mailbuffer[7] = 0;

	// send the message
	mbox_write(MBOX_PROP, mb_addr);

	// read the response
	mbox_read(MBOX_PROP);

	if(mailbuffer[1] != MBOX_SUCCESS)
	{
	    printf("EMMC: bcm_2708_power_off(): property mailbox did not return a valid response.\n");
	    return -1;
	}

	if(mailbuffer[5] != 0x0)
	{
	    printf("EMMC: property mailbox did not return a valid device id.\n");
	    return -1;
	}

	if((mailbuffer[6] & 0x3) != 0)
	{
#ifdef EMMC_DEBUG
		printf("EMMC: bcm_2708_power_off(): device did not power off successfully (%08x).\n", mailbuffer[6]);
#endif
		return 1;
	}

	return 0;
}

static int bcm_2708_power_on()
{
	uintptr_t mb_addr = 0x40007000;		// 0x7000 in L2 cache coherent mode
	volatile u32 *mailbuffer = (u32 *)mb_addr;

	/* Power on the SD card */
	// set up the buffer
	mailbuffer[0] = 8 * 4;		// size of this message
	mailbuffer[1] = 0;			// this is a request

	// next comes the first tag
	mailbuffer[2] = 0x00028001;	// set power state tag
	mailbuffer[3] = 0x8;		// value buffer size
	mailbuffer[4] = 0x8;		// is a request, value length = 8
	mailbuffer[5] = 0x0;		// device id and device id also returned here
	mailbuffer[6] = 0x3;		// set power off, wait for stable and returns state

	// closing tag
	mailbuffer[7] = 0;

	// send the message
	mbox_write(MBOX_PROP, mb_addr);

	// read the response
	mbox_read(MBOX_PROP);

	if(mailbuffer[1] != MBOX_SUCCESS)
	{
	    printf("EMMC: bcm_2708_power_on(): property mailbox did not return a valid response.\n");
	    return -1;
	}

	if(mailbuffer[5] != 0x0)
	{
	    printf("EMMC: property mailbox did not return a valid device id.\n");
	    return -1;
	}

	if((mailbuffer[6] & 0x3) != 1)
	{
#ifdef EMMC_DEBUG
		printf("EMMC: bcm_2708_power_on(): device did not power on successfully (%08x).\n", mailbuffer[6]);
#endif
		return 1;
	}

	return 0;
}

static int bcm_2708_power_cycle()
{
	if(bcm_2708_power_off() < 0)
		return -1;

	usleep(5000);

	return bcm_2708_power_on();
}
#endif

// Set the clock dividers to generate a target value
static u32 sd_get_clock_divider(u32 base_clock, u32 target_rate)
{
    // TODO: implement use of preset value registers

    u32 targetted_divisor = 0;
    if(target_rate > base_clock)
        targetted_divisor = 1;
    else
    {
        targetted_divisor = base_clock / target_rate;
        u32 mod = base_clock % target_rate;
        if(mod)
            targetted_divisor--;
    }

    // Decide on the clock mode to use

    // Currently only 10-bit divided clock mode is supported

#ifndef EMMC_ALLOW_OLD_SDHCI
    if(hci_ver >= 2)
    {
#endif
        // HCI version 3 or greater supports 10-bit divided clock mode
        // This requires a power-of-two divider

        // Find the first bit set
        int divisor = -1;
        for(int first_bit = 31; first_bit >= 0; first_bit--)
        {
            u32 bit_test = (1 << first_bit);
            if(targetted_divisor & bit_test)
            {
                divisor = first_bit;
                targetted_divisor &= ~bit_test;
                if(targetted_divisor)
                {
                    // The divisor is not a power-of-two, increase it
                    divisor++;
                }
                break;
            }
        }

        if(divisor == -1)
            divisor = 31;
        if(divisor >= 32)
            divisor = 31;

        if(divisor != 0)
            divisor = (1 << (divisor - 1));

        if(divisor >= 0x400)
            divisor = 0x3ff;

        u32 freq_select = divisor & 0xff;
        u32 upper_bits = (divisor >> 8) & 0x3;
        u32 ret = (freq_select << 8) | (upper_bits << 6) | (0 << 5);

#ifdef EMMC_DEBUG
        int denominator = 1;
        if(divisor != 0)
            denominator = divisor * 2;
        int actual_clock = base_clock / denominator;
        printf("EMMC: base_clock: %i, target_rate: %i, divisor: %08x, "
               "actual_clock: %i, ret: %08x\n", base_clock, target_rate,
               divisor, actual_clock, ret);
#endif

        return ret;
#ifndef EMMC_ALLOW_OLD_SDHCI
    }
    else
    {
        printf("EMMC: unsupported host version\n");
        return SD_GET_CLOCK_DIVIDER_FAIL;
    }
#endif

}

// Switch the clock rate whilst running
static int sd_switch_clock_rate(u32 base_clock, u32 target_rate)
{
    // Decide on an appropriate divider
    u32 divider = sd_get_clock_divider(base_clock, target_rate);
    if(divider == SD_GET_CLOCK_DIVIDER_FAIL)
    {
        printf("EMMC: couldn't get a valid divider for target rate %i Hz\n",
               target_rate);
        return -1;
    }

    // Wait for the command inhibit (CMD and DAT) bits to clear
    while(mmio_read(emmc_base + EMMC_STATUS) & 0x3)
        usleep(1000);

    // Set the SD clock off
    u32 control1 = mmio_read(emmc_base + EMMC_CONTROL1);
    control1 &= ~(1 << 2);
    mmio_write(emmc_base + EMMC_CONTROL1, control1);
    usleep(2000);

    // Write the new divider
	control1 &= ~0xffe0;		// Clear old setting + clock generator select
    control1 |= divider;
    mmio_write(emmc_base + EMMC_CONTROL1, control1);
    usleep(2000);

    // Enable the SD clock
    control1 |= (1 << 2);
    mmio_write(emmc_base + EMMC_CONTROL1, control1);
    usleep(2000);

#ifdef EMMC_DEBUG
    printf("EMMC: successfully set clock rate to %i Hz\n", target_rate);
#endif
    return 0;
}

// Reset the CMD line
static int sd_reset_cmd()
{
    u32 control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	control1 |= SD_RESET_CMD;
	mmio_write(emmc_base + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT((mmio_read(emmc_base + EMMC_CONTROL1) & SD_RESET_CMD) == 0, 1000000);
	if((mmio_read(emmc_base + EMMC_CONTROL1) & SD_RESET_CMD) != 0)
	{
		printf("EMMC: CMD line did not reset properly\n");
		return -1;
	}
	return 0;
}

// Reset the CMD line
static int sd_reset_dat()
{
    u32 control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	control1 |= SD_RESET_DAT;
	mmio_write(emmc_base + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT((mmio_read(emmc_base + EMMC_CONTROL1) & SD_RESET_DAT) == 0, 1000000);
	if((mmio_read(emmc_base + EMMC_CONTROL1) & SD_RESET_DAT) != 0)
	{
		printf("EMMC: DAT line did not reset properly\n");
		return -1;
	}
	return 0;
}

static void sd_issue_command_int(struct emmc_block_dev *dev, u32 cmd_reg, u32 argument, useconds_t timeout)
{
    dev->last_cmd_reg = cmd_reg;
    dev->last_cmd_success = 0;

    // This is as per HCSS 3.7.1.1/3.7.2.2

    // Check Command Inhibit
    while(mmio_read(emmc_base + EMMC_STATUS) & 0x1)
        usleep(1000);

    // Is the command with busy?
    if((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B)
    {
        // With busy

        // Is is an abort command?
        if((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT)
        {
            // Not an abort command

            // Wait for the data line to be free
            while(mmio_read(emmc_base + EMMC_STATUS) & 0x2)
                usleep(1000);
        }
    }

    // Is this a DMA transfer?
    int is_sdma = 0;
    if((cmd_reg & SD_CMD_ISDATA) && (dev->use_sdma))
    {
#ifdef EMMC_DEBUG
        printf("SD: performing SDMA transfer, current INTERRUPT: %08x\n",
               mmio_read(emmc_base + EMMC_INTERRUPT));
#endif
        is_sdma = 1;
    }

    if(is_sdma)
    {
        // Set system address register (ARGUMENT2 in RPi)

        // We need to define a 4 kiB aligned buffer to use here
        // Then convert its virtual address to a bus address
        mmio_write(emmc_base + EMMC_ARG2, SDMA_BUFFER_PA);
    }

    // Set block size and block count
    // For now, block size = 512 bytes, block count = 1,
    //  host SDMA buffer boundary = 4 kiB
    if(dev->blocks_to_transfer > 0xffff)
    {
        printf("SD: blocks_to_transfer too great (%i)\n",
               dev->blocks_to_transfer);
        dev->last_cmd_success = 0;
        return;
    }
    u32 blksizecnt = dev->block_size | (dev->blocks_to_transfer << 16);
    mmio_write(emmc_base + EMMC_BLKSIZECNT, blksizecnt);

    // Set argument 1 reg
    mmio_write(emmc_base + EMMC_ARG1, argument);

    if(is_sdma)
    {
        // Set Transfer mode register
        cmd_reg |= SD_CMD_DMA;
    }

    // Set command reg
    mmio_write(emmc_base + EMMC_CMDTM, cmd_reg);

    usleep(2000);

    // Wait for command complete interrupt
    TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_INTERRUPT) & 0x8001, timeout);
    u32 irpts = mmio_read(emmc_base + EMMC_INTERRUPT);

    // Clear command complete status
    mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff0001);

    // Test for errors
    if((irpts & 0xffff0001) != 0x1)
    {
#ifdef EMMC_DEBUG
        printf("SD: error occured whilst waiting for command complete interrupt\n");
#endif
        dev->last_error = irpts & 0xffff0000;
        dev->last_interrupt = irpts;
        return;
    }

    usleep(2000);

    // Get response data
    switch(cmd_reg & SD_CMD_RSPNS_TYPE_MASK)
    {
        case SD_CMD_RSPNS_TYPE_48:
        case SD_CMD_RSPNS_TYPE_48B:
            dev->last_r0 = mmio_read(emmc_base + EMMC_RESP0);
            break;

        case SD_CMD_RSPNS_TYPE_136:
            dev->last_r0 = mmio_read(emmc_base + EMMC_RESP0);
            dev->last_r1 = mmio_read(emmc_base + EMMC_RESP1);
            dev->last_r2 = mmio_read(emmc_base + EMMC_RESP2);
            dev->last_r3 = mmio_read(emmc_base + EMMC_RESP3);
            break;
    }

    // If with data, wait for the appropriate interrupt
    if((cmd_reg & SD_CMD_ISDATA) && (is_sdma == 0))
    {
        u32 wr_irpt;
        int is_write = 0;
        if(cmd_reg & SD_CMD_DAT_DIR_CH)
            wr_irpt = (1 << 5);     // read
        else
        {
            is_write = 1;
            wr_irpt = (1 << 4);     // write
        }

        int cur_block = 0;
        u32 *cur_buf_addr = (u32 *)dev->buf;
        while(cur_block < dev->blocks_to_transfer)
        {
#ifdef EMMC_DEBUG
			if(dev->blocks_to_transfer > 1)
				printf("SD: multi block transfer, awaiting block %i ready\n",
				cur_block);
#endif
            TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_INTERRUPT) & (wr_irpt | 0x8000), timeout);
            irpts = mmio_read(emmc_base + EMMC_INTERRUPT);
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff0000 | wr_irpt);

            if((irpts & (0xffff0000 | wr_irpt)) != wr_irpt)
            {
#ifdef EMMC_DEBUG
            printf("SD: error occured whilst waiting for data ready interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            // Transfer the block
            size_t cur_byte_no = 0;
            while(cur_byte_no < dev->block_size)
            {
                if(is_write)
				{
					u32 data = read_word((uint8_t *)cur_buf_addr, 0);
                    mmio_write(emmc_base + EMMC_DATA, data);
				}
                else
				{
					u32 data = mmio_read(emmc_base + EMMC_DATA);
					write_word(data, (uint8_t *)cur_buf_addr, 0);
				}
                cur_byte_no += 4;
                cur_buf_addr++;
            }

#ifdef EMMC_DEBUG
			printf("SD: block %i transfer complete\n", cur_block);
#endif

            cur_block++;
        }
    }

    // Wait for transfer complete (set if read/write transfer or with busy)
    if((((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||
       (cmd_reg & SD_CMD_ISDATA)) && (is_sdma == 0))
    {
        // First check command inhibit (DAT) is not already 0
        if((mmio_read(emmc_base + EMMC_STATUS) & 0x2) == 0)
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff0002);
        else
        {
            TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_INTERRUPT) & 0x8002, timeout);
            irpts = mmio_read(emmc_base + EMMC_INTERRUPT);
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff0002);

            // Handle the case where both data timeout and transfer complete
            //  are set - transfer complete overrides data timeout: HCSS 2.2.17
            if(((irpts & 0xffff0002) != 0x2) && ((irpts & 0xffff0002) != 0x100002))
            {
#ifdef EMMC_DEBUG
                printf("SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff0002);
        }
    }
    else if (is_sdma)
    {
        // For SDMA transfers, we have to wait for either transfer complete,
        //  DMA int or an error

        // First check command inhibit (DAT) is not already 0
        if((mmio_read(emmc_base + EMMC_STATUS) & 0x2) == 0)
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff000a);
        else
        {
            TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_INTERRUPT) & 0x800a, timeout);
            irpts = mmio_read(emmc_base + EMMC_INTERRUPT);
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff000a);

            // Detect errors
            if((irpts & 0x8000) && ((irpts & 0x2) != 0x2))
            {
#ifdef EMMC_DEBUG
                printf("SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            // Detect DMA interrupt without transfer complete
            // Currently not supported - all block sizes should fit in the
            //  buffer
            if((irpts & 0x8) && ((irpts & 0x2) != 0x2))
            {
#ifdef EMMC_DEBUG
                printf("SD: error: DMA interrupt occured without transfer complete\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            // Detect transfer complete
            if(irpts & 0x2)
            {
#ifdef EMMC_DEBUG
                printf("SD: SDMA transfer complete");
#endif
                // Transfer the data to the user buffer
                memcpy(dev->buf, (const void *)SDMA_BUFFER, dev->block_size);
            }
            else
            {
                // Unknown error
#ifdef EMMC_DEBUG
                if(irpts == 0)
                    printf("SD: timeout waiting for SDMA transfer to complete\n");
                else
                    printf("SD: unknown SDMA transfer error\n");

                printf("SD: INTERRUPT: %08x, STATUS %08x\n", irpts,
                       mmio_read(emmc_base + EMMC_STATUS));
#endif

                if((irpts == 0) && ((mmio_read(emmc_base + EMMC_STATUS) & 0x3) == 0x2))
                {
                    // The data transfer is ongoing, we should attempt to stop
                    //  it
#ifdef EMMC_DEBUG
                    printf("SD: aborting transfer\n");
#endif
                    mmio_write(emmc_base + EMMC_CMDTM, sd_commands[STOP_TRANSMISSION]);

#ifdef EMMC_DEBUG
                    // pause to let us read the screen
                    usleep(2000000);
#endif
                }
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }
        }
    }

    // Return success
    dev->last_cmd_success = 1;
}

static void sd_handle_card_interrupt(struct emmc_block_dev *dev)
{
    // Handle a card interrupt

#ifdef EMMC_DEBUG
    u32 status = mmio_read(emmc_base + EMMC_STATUS);

    printf("SD: card interrupt\n");
    printf("SD: controller status: %08x\n", status);
#endif

    // Get the card status
    if(dev->card_rca)
    {
        sd_issue_command_int(dev, sd_commands[SEND_STATUS], dev->card_rca << 16,
                         500000);
        if(FAIL(dev))
        {
#ifdef EMMC_DEBUG
            printf("SD: unable to get card status\n");
#endif
        }
        else
        {
#ifdef EMMC_DEBUG
            printf("SD: card status: %08x\n", dev->last_r0);
#endif
        }
    }
    else
    {
#ifdef EMMC_DEBUG
        printf("SD: no card currently selected\n");
#endif
    }
}

static void sd_handle_interrupts(struct emmc_block_dev *dev)
{
    u32 irpts = mmio_read(emmc_base + EMMC_INTERRUPT);
    u32 reset_mask = 0;

    if(irpts & SD_COMMAND_COMPLETE)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious command complete interrupt\n");
#endif
        reset_mask |= SD_COMMAND_COMPLETE;
    }

    if(irpts & SD_TRANSFER_COMPLETE)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious transfer complete interrupt\n");
#endif
        reset_mask |= SD_TRANSFER_COMPLETE;
    }

    if(irpts & SD_BLOCK_GAP_EVENT)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious block gap event interrupt\n");
#endif
        reset_mask |= SD_BLOCK_GAP_EVENT;
    }

    if(irpts & SD_DMA_INTERRUPT)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious DMA interrupt\n");
#endif
        reset_mask |= SD_DMA_INTERRUPT;
    }

    if(irpts & SD_BUFFER_WRITE_READY)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer write ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_WRITE_READY;
        sd_reset_dat();
    }

    if(irpts & SD_BUFFER_READ_READY)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer read ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_READ_READY;
        sd_reset_dat();
    }

    if(irpts & SD_CARD_INSERTION)
    {
#ifdef EMMC_DEBUG
        printf("SD: card insertion detected\n");
#endif
        reset_mask |= SD_CARD_INSERTION;
    }

    if(irpts & SD_CARD_REMOVAL)
    {
#ifdef EMMC_DEBUG
        printf("SD: card removal detected\n");
#endif
        reset_mask |= SD_CARD_REMOVAL;
        dev->card_removal = 1;
    }

    if(irpts & SD_CARD_INTERRUPT)
    {
#ifdef EMMC_DEBUG
        printf("SD: card interrupt detected\n");
#endif
        sd_handle_card_interrupt(dev);
        reset_mask |= SD_CARD_INTERRUPT;
    }

    if(irpts & 0x8000)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious error interrupt: %08x\n", irpts);
#endif
        reset_mask |= 0xffff0000;
    }

    mmio_write(emmc_base + EMMC_INTERRUPT, reset_mask);
}

static void sd_issue_command(struct emmc_block_dev *dev, u32 command, u32 argument, useconds_t timeout)
{
    // First, handle any pending interrupts
    sd_handle_interrupts(dev);

    // Stop the command issue if it was the card remove interrupt that was
    //  handled
    if(dev->card_removal)
    {
        dev->last_cmd_success = 0;
        return;
    }

    // Now run the appropriate commands by calling sd_issue_command_int()
    if(command & IS_APP_CMD)
    {
        command &= 0xff;
#ifdef EMMC_DEBUG
        printf("SD: issuing command ACMD%i\n", command);
#endif

        if(sd_acommands[command] == SD_CMD_RESERVED(0))
        {
            printf("SD: invalid command ACMD%i\n", command);
            dev->last_cmd_success = 0;
            return;
        }
        dev->last_cmd = APP_CMD;

        u32 rca = 0;
        if(dev->card_rca)
            rca = dev->card_rca << 16;
        sd_issue_command_int(dev, sd_commands[APP_CMD], rca, timeout);
        if(dev->last_cmd_success)
        {
            dev->last_cmd = command | IS_APP_CMD;
            sd_issue_command_int(dev, sd_acommands[command], argument, timeout);
        }
    }
    else
    {
#ifdef EMMC_DEBUG
        printf("SD: issuing command CMD%i\n", command);
#endif

        if(sd_commands[command] == SD_CMD_RESERVED(0))
        {
            printf("SD: invalid command CMD%i\n", command);
            dev->last_cmd_success = 0;
            return;
        }

        dev->last_cmd = command;
        sd_issue_command_int(dev, sd_commands[command], argument, timeout);
    }

#ifdef EMMC_DEBUG
    if(FAIL(dev))
    {
        printf("SD: error issuing command: interrupts %08x: ", dev->last_interrupt);
        if(dev->last_error == 0)
            printf("TIMEOUT");
        else
        {
            for(int i = 0; i < SD_ERR_RSVD; i++)
            {
                if(dev->last_error & (1 << (i + 16)))
                {
                    printf(err_irpts[i]);
                    printf(" ");
                }
            }
        }
		printf("\n");
    }
    else
        printf("SD: command completed successfully\n");
#endif
}

int sd_card_init(struct block_device **dev)
{
    // Check the sanity of the sd_commands and sd_acommands structures
    if(sizeof(sd_commands) != (64 * sizeof(u32)))
    {
        printf("EMMC: fatal error, sd_commands of incorrect size: %i"
               " expected %i\n", sizeof(sd_commands),
               64 * sizeof(u32));
        return -1;
    }
    if(sizeof(sd_acommands) != (64 * sizeof(u32)))
    {
        printf("EMMC: fatal error, sd_acommands of incorrect size: %i"
               " expected %i\n", sizeof(sd_acommands),
               64 * sizeof(u32));
        return -1;
    }

#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
	// Power cycle the card to ensure its in its startup state
	if(bcm_2708_power_cycle() != 0)
	{
		printf("EMMC: BCM2708 controller did not power cycle successfully\n");
	}
#ifdef EMMC_DEBUG
	else
		printf("EMMC: BCM2708 controller power-cycled\n");
#endif
#endif

	// Read the controller version
	u32 ver = mmio_read(emmc_base + EMMC_SLOTISR_VER);
	u32 vendor = ver >> 24;
	u32 sdversion = (ver >> 16) & 0xff;
	u32 slot_status = ver & 0xff;
	printf("EMMC: vendor %x, sdversion %x, slot_status %x\n", vendor, sdversion, slot_status);
	hci_ver = sdversion;

	if(hci_ver < 2)
	{
#ifdef EMMC_ALLOW_OLD_SDHCI
		printf("EMMC: WARNING: old SDHCI version detected\n");
#else
		printf("EMMC: only SDHCI versions >= 3.0 are supported\n");
		return -1;
#endif
	}

	// Reset the controller
#ifdef EMMC_DEBUG
	printf("EMMC: resetting controller\n");
#endif
	u32 control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	control1 |= (1 << 24);
	// Disable clock
	control1 &= ~(1 << 2);
	control1 &= ~(1 << 0);
	mmio_write(emmc_base + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT((mmio_read(emmc_base + EMMC_CONTROL1) & (0x7 << 24)) == 0, 1000000);
	if((mmio_read(emmc_base + EMMC_CONTROL1) & (0x7 << 24)) != 0)
	{
		printf("EMMC: controller did not reset properly\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: control0: %08x, control1: %08x, control2: %08x\n",
			mmio_read(emmc_base + EMMC_CONTROL0),
			mmio_read(emmc_base + EMMC_CONTROL1),
            mmio_read(emmc_base + EMMC_CONTROL2));
#endif

	// Read the capabilities registers
	capabilities_0 = mmio_read(emmc_base + EMMC_CAPABILITIES_0);
	capabilities_1 = mmio_read(emmc_base + EMMC_CAPABILITIES_1);
#ifdef EMMC_DEBUG
	printf("EMMC: capabilities: %08x%08x\n", capabilities_1, capabilities_0);
#endif

	// Check for a valid card
#ifdef EMMC_DEBUG
	printf("EMMC: checking for an inserted card\n");
#endif
    TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_STATUS) & (1 << 16), 500000);
	u32 status_reg = mmio_read(emmc_base + EMMC_STATUS);
	if((status_reg & (1 << 16)) == 0)
	{
		printf("EMMC: no card inserted\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: status: %08x\n", status_reg);
#endif

	// Clear control2
	mmio_write(emmc_base + EMMC_CONTROL2, 0);

	// Get the base clock rate
	u32 base_clock = sd_get_base_clock_hz();
	if(base_clock == 0)
	{
	    printf("EMMC: assuming clock rate to be 100MHz\n");
	    base_clock = 100000000;
	}

	// Set clock rate to something slow
#ifdef EMMC_DEBUG
	printf("EMMC: setting clock rate\n");
#endif
	control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	control1 |= 1;			// enable clock

	// Set to identification frequency (400 kHz)
	u32 f_id = sd_get_clock_divider(base_clock, SD_CLOCK_ID);
	if(f_id == SD_GET_CLOCK_DIVIDER_FAIL)
	{
		printf("EMMC: unable to get a valid clock divider for ID frequency\n");
		return -1;
	}
	control1 |= f_id;

	control1 |= (7 << 16);		// data timeout = TMCLK * 2^10
	mmio_write(emmc_base + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_CONTROL1) & 0x2, 0x1000000);
	if((mmio_read(emmc_base + EMMC_CONTROL1) & 0x2) == 0)
	{
		printf("EMMC: controller's clock did not stabilise within 1 second\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: control0: %08x, control1: %08x\n",
			mmio_read(emmc_base + EMMC_CONTROL0),
			mmio_read(emmc_base + EMMC_CONTROL1));
#endif

	// Enable the SD clock
#ifdef EMMC_DEBUG
	printf("EMMC: enabling SD clock\n");
#endif
	usleep(2000);
	control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	control1 |= 4;
	mmio_write(emmc_base + EMMC_CONTROL1, control1);
	usleep(2000);
#ifdef EMMC_DEBUG
	printf("EMMC: SD clock enabled\n");
#endif

	// Mask off sending interrupts to the ARM
	mmio_write(emmc_base + EMMC_IRPT_EN, 0);
	// Reset interrupts
	mmio_write(emmc_base + EMMC_INTERRUPT, 0xffffffff);
	// Have all interrupts sent to the INTERRUPT register
	u32 irpt_mask = 0xffffffff & (~SD_CARD_INTERRUPT);
#ifdef SD_CARD_INTERRUPTS
    irpt_mask |= SD_CARD_INTERRUPT;
#endif
	mmio_write(emmc_base + EMMC_IRPT_MASK, irpt_mask);

#ifdef EMMC_DEBUG
	printf("EMMC: interrupts disabled\n");
#endif
	usleep(2000);

    // Prepare the device structure
	struct emmc_block_dev *ret;
	if(*dev == NULL)
		ret = (struct emmc_block_dev *)malloc(sizeof(struct emmc_block_dev));
	else
		ret = (struct emmc_block_dev *)*dev;

	assert(ret);

	memset(ret, 0, sizeof(struct emmc_block_dev));
	ret->bd.driver_name = driver_name;
	ret->bd.device_name = device_name;
	ret->bd.block_size = 512;
	ret->bd.read = sd_read;
#ifdef SD_WRITE_SUPPORT
    ret->bd.write = sd_write;
#endif
    ret->bd.supports_multiple_block_read = 1;
    ret->bd.supports_multiple_block_write = 1;
	ret->base_clock = base_clock;

#ifdef EMMC_DEBUG
	printf("EMMC: device structure created\n");
#endif

	// Send CMD0 to the card (reset to idle state)
	sd_issue_command(ret, GO_IDLE_STATE, 0, 500000);
	if(FAIL(ret))
	{
        printf("SD: no CMD0 response\n");
        return -1;
	}

	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
#ifdef EMMC_DEBUG
    printf("SD: note a timeout error on the following command (CMD8) is normal "
           "and expected if the SD card version is less than 2.0\n");
#endif
	sd_issue_command(ret, SEND_IF_COND, 0x1aa, 500000);
	int v2_later = 0;
	if(TIMEOUT(ret))
        v2_later = 0;
    else if(CMD_TIMEOUT(ret))
    {
        if(sd_reset_cmd() == -1)
            return -1;
        mmio_write(emmc_base + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        v2_later = 0;
    }
    else if(FAIL(ret))
    {
        printf("SD: failure sending CMD8 (%08x)\n", ret->last_interrupt);
        return -1;
    }
    else
    {
        if((ret->last_r0 & 0xfff) != 0x1aa)
        {
            printf("SD: unusable card\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD8 response %08x\n", ret->last_r0);
#endif
            return -1;
        }
        else
            v2_later = 1;
    }

    // Here we are supposed to check the response to CMD5 (HCSS 3.6)
    // It only returns if the card is a SDIO card
#ifdef EMMC_DEBUG
    printf("SD: note that a timeout error on the following command (CMD5) is "
           "normal and expected if the card is not a SDIO card.\n");
#endif
    sd_issue_command(ret, IO_SET_OP_COND, 0, 10000);
    if(!TIMEOUT(ret))
    {
        if(CMD_TIMEOUT(ret))
        {
            if(sd_reset_cmd() == -1)
                return -1;
            mmio_write(emmc_base + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        }
        else
        {
            printf("SD: SDIO card detected - not currently supported\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD5 returned %08x\n", ret->last_r0);
#endif
            return -1;
        }
    }

    // Call an inquiry ACMD41 (voltage window = 0) to get the OCR
#ifdef EMMC_DEBUG
    printf("SD: sending inquiry ACMD41\n");
#endif
    sd_issue_command(ret, ACMD(41), 0, 500000);
    if(FAIL(ret))
    {
        printf("SD: inquiry ACMD41 failed\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    printf("SD: inquiry ACMD41 returned %08x\n", ret->last_r0);
#endif

	// Call initialization ACMD41
	int card_is_busy = 1;
	while(card_is_busy)
	{
	    u32 v2_flags = 0;
	    if(v2_later)
	    {
	        // Set SDHC support
	        v2_flags |= (1 << 30);

	        // Set 1.8v support
#ifdef SD_1_8V_SUPPORT
	        if(!ret->failed_voltage_switch)
                v2_flags |= (1 << 24);
#endif

            // Enable SDXC maximum performance
#ifdef SDXC_MAXIMUM_PERFORMANCE
            v2_flags |= (1 << 28);
#endif
	    }

	    sd_issue_command(ret, ACMD(41), 0x00ff8000 | v2_flags, 500000);
	    if(FAIL(ret))
	    {
	        printf("SD: error issuing ACMD41\n");
	        return -1;
	    }

	    if((ret->last_r0 >> 31) & 0x1)
	    {
	        // Initialization is complete
	        ret->card_ocr = (ret->last_r0 >> 8) & 0xffff;
	        ret->card_supports_sdhc = (ret->last_r0 >> 30) & 0x1;

#ifdef SD_1_8V_SUPPORT
	        if(!ret->failed_voltage_switch)
                ret->card_supports_18v = (ret->last_r0 >> 24) & 0x1;
#endif

	        card_is_busy = 0;
	    }
	    else
	    {
	        // Card is still busy
#ifdef EMMC_DEBUG
            printf("SD: card is busy, retrying\n");
#endif
            usleep(500000);
	    }
	}

#ifdef EMMC_DEBUG
	printf("SD: card identified: OCR: %04x, 1.8v support: %i, SDHC support: %i\n",
			ret->card_ocr, ret->card_supports_18v, ret->card_supports_sdhc);
#endif

    // At this point, we know the card is definitely an SD card, so will definitely
	//  support SDR12 mode which runs at 25 MHz
    sd_switch_clock_rate(base_clock, SD_CLOCK_NORMAL);

	// A small wait before the voltage switch
	usleep(5000);

	// Switch to 1.8V mode if possible
	if(ret->card_supports_18v)
	{
#ifdef EMMC_DEBUG
        printf("SD: switching to 1.8V mode\n");
#endif
	    // As per HCSS 3.6.1

	    // Send VOLTAGE_SWITCH
	    sd_issue_command(ret, VOLTAGE_SWITCH, 0, 500000);
	    if(FAIL(ret))
	    {
#ifdef EMMC_DEBUG
            printf("SD: error issuing VOLTAGE_SWITCH\n");
#endif
	        ret->failed_voltage_switch = 1;
			sd_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Disable SD clock
	    control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	    control1 &= ~(1 << 2);
	    mmio_write(emmc_base + EMMC_CONTROL1, control1);

	    // Check DAT[3:0]
	    status_reg = mmio_read(emmc_base + EMMC_STATUS);
	    u32 dat30 = (status_reg >> 20) & 0xf;
	    if(dat30 != 0)
	    {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 0\n");
#endif
	        ret->failed_voltage_switch = 1;
			sd_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Set 1.8V signal enable to 1
	    u32 control0 = mmio_read(emmc_base + EMMC_CONTROL0);
	    control0 |= (1 << 8);
	    mmio_write(emmc_base + EMMC_CONTROL0, control0);

	    // Wait 5 ms
	    usleep(5000);

	    // Check the 1.8V signal enable is set
	    control0 = mmio_read(emmc_base + EMMC_CONTROL0);
	    if(((control0 >> 8) & 0x1) == 0)
	    {
#ifdef EMMC_DEBUG
            printf("SD: controller did not keep 1.8V signal enable high\n");
#endif
	        ret->failed_voltage_switch = 1;
			sd_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Re-enable the SD clock
	    control1 = mmio_read(emmc_base + EMMC_CONTROL1);
	    control1 |= (1 << 2);
	    mmio_write(emmc_base + EMMC_CONTROL1, control1);

	    // Wait 1 ms
	    usleep(10000);

	    // Check DAT[3:0]
	    status_reg = mmio_read(emmc_base + EMMC_STATUS);
	    dat30 = (status_reg >> 20) & 0xf;
	    if(dat30 != 0xf)
	    {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 1111b (%01x)\n", dat30);
#endif
	        ret->failed_voltage_switch = 1;
			sd_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

#ifdef EMMC_DEBUG
        printf("SD: voltage switch complete\n");
#endif
	}

	// Send CMD2 to get the cards CID
	sd_issue_command(ret, ALL_SEND_CID, 0, 500000);
	if(FAIL(ret))
	{
	    printf("SD: error sending ALL_SEND_CID\n");
	    return -1;
	}
	u32 card_cid_0 = ret->last_r0;
	u32 card_cid_1 = ret->last_r1;
	u32 card_cid_2 = ret->last_r2;
	u32 card_cid_3 = ret->last_r3;

#ifdef EMMC_DEBUG
	printf("SD: card CID: %08x%08x%08x%08x\n", card_cid_3, card_cid_2, card_cid_1, card_cid_0);
#endif
	u32 *dev_id = (u32 *)malloc(4 * sizeof(u32));
	dev_id[0] = card_cid_0;
	dev_id[1] = card_cid_1;
	dev_id[2] = card_cid_2;
	dev_id[3] = card_cid_3;
	ret->bd.device_id = (uint8_t *)dev_id;
	ret->bd.dev_id_len = 4 * sizeof(u32);

	// Send CMD3 to enter the data state
	sd_issue_command(ret, SEND_RELATIVE_ADDR, 0, 500000);
	if(FAIL(ret))
    {
        printf("SD: error sending SEND_RELATIVE_ADDR\n");
        free(ret);
        return -1;
    }

	u32 cmd3_resp = ret->last_r0;
#ifdef EMMC_DEBUG
	printf("SD: CMD3 response: %08x\n", cmd3_resp);
#endif

	ret->card_rca = (cmd3_resp >> 16) & 0xffff;
	u32 crc_error = (cmd3_resp >> 15) & 0x1;
	u32 illegal_cmd = (cmd3_resp >> 14) & 0x1;
	u32 error = (cmd3_resp >> 13) & 0x1;
	u32 status = (cmd3_resp >> 9) & 0xf;
	u32 ready = (cmd3_resp >> 8) & 0x1;

	if(crc_error)
	{
		printf("SD: CRC error\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(illegal_cmd)
	{
		printf("SD: illegal command\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(error)
	{
		printf("SD: generic error\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(!ready)
	{
		printf("SD: not ready for data\n");
		free(ret);
		free(dev_id);
		return -1;
	}

#ifdef EMMC_DEBUG
	printf("SD: RCA: %04x\n", ret->card_rca);
#endif

	// Now select the card (toggles it to transfer state)
	sd_issue_command(ret, SELECT_CARD, ret->card_rca << 16, 500000);
	if(FAIL(ret))
	{
	    printf("SD: error sending CMD7\n");
	    free(ret);
	    return -1;
	}

	u32 cmd7_resp = ret->last_r0;
	status = (cmd7_resp >> 9) & 0xf;

	if((status != 3) && (status != 4))
	{
		printf("SD: invalid status (%i)\n", status);
		free(ret);
		free(dev_id);
		return -1;
	}

	// If not an SDHC card, ensure BLOCKLEN is 512 bytes
	if(!ret->card_supports_sdhc)
	{
	    sd_issue_command(ret, SET_BLOCKLEN, 512, 500000);
	    if(FAIL(ret))
	    {
	        printf("SD: error sending SET_BLOCKLEN\n");
	        free(ret);
	        return -1;
	    }
	}
	ret->block_size = 512;
	u32 controller_block_size = mmio_read(emmc_base + EMMC_BLKSIZECNT);
	controller_block_size &= (~0xfff);
	controller_block_size |= 0x200;
	mmio_write(emmc_base + EMMC_BLKSIZECNT, controller_block_size);

	// Get the cards SCR register
	ret->scr = (struct sd_scr *)malloc(sizeof(struct sd_scr));
	ret->buf = &ret->scr->scr[0];
	ret->block_size = 8;
	ret->blocks_to_transfer = 1;
	sd_issue_command(ret, SEND_SCR, 0, 500000);
	ret->block_size = 512;
	if(FAIL(ret))
	{
	    printf("SD: error sending SEND_SCR\n");
	    free(ret->scr);
        free(ret);
	    return -1;
	}

	// Determine card version
	// Note that the SCR is big-endian
	u32 scr0 = byte_swap(ret->scr->scr[0]);
	ret->scr->sd_version = SD_VER_UNKNOWN;
	u32 sd_spec = (scr0 >> (56 - 32)) & 0xf;
	u32 sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
	u32 sd_spec4 = (scr0 >> (42 - 32)) & 0x1;
	ret->scr->sd_bus_widths = (scr0 >> (48 - 32)) & 0xf;
	if(sd_spec == 0)
        ret->scr->sd_version = SD_VER_1;
    else if(sd_spec == 1)
        ret->scr->sd_version = SD_VER_1_1;
    else if(sd_spec == 2)
    {
        if(sd_spec3 == 0)
            ret->scr->sd_version = SD_VER_2;
        else if(sd_spec3 == 1)
        {
            if(sd_spec4 == 0)
                ret->scr->sd_version = SD_VER_3;
            else if(sd_spec4 == 1)
                ret->scr->sd_version = SD_VER_4;
        }
    }

#ifdef EMMC_DEBUG
    printf("SD: &scr: %08x\n", &ret->scr->scr[0]);
    printf("SD: SCR[0]: %08x, SCR[1]: %08x\n", ret->scr->scr[0], ret->scr->scr[1]);;
    printf("SD: SCR: %08x%08x\n", byte_swap(ret->scr->scr[0]), byte_swap(ret->scr->scr[1]));
    printf("SD: SCR: version %s, bus_widths %01x\n", sd_versions[ret->scr->sd_version],
           ret->scr->sd_bus_widths);
#endif

    if(ret->scr->sd_bus_widths & 0x4)
    {
        // Set 4-bit transfer mode (ACMD6)
        // See HCSS 3.4 for the algorithm
#ifdef SD_4BIT_DATA
#ifdef EMMC_DEBUG
        printf("SD: switching to 4-bit data mode\n");
#endif

        // Disable card interrupt in host
        u32 old_irpt_mask = mmio_read(emmc_base + EMMC_IRPT_MASK);
        u32 new_iprt_mask = old_irpt_mask & ~(1 << 8);
        mmio_write(emmc_base + EMMC_IRPT_MASK, new_iprt_mask);

        // Send ACMD6 to change the card's bit mode
        sd_issue_command(ret, SET_BUS_WIDTH, 0x2, 500000);
        if(FAIL(ret))
            printf("SD: switch to 4-bit data mode failed\n");
        else
        {
            // Change bit mode for Host
            u32 control0 = mmio_read(emmc_base + EMMC_CONTROL0);
            control0 |= 0x2;
            mmio_write(emmc_base + EMMC_CONTROL0, control0);

            // Re-enable card interrupt in host
            mmio_write(emmc_base + EMMC_IRPT_MASK, old_irpt_mask);

#ifdef EMMC_DEBUG
                printf("SD: switch to 4-bit complete\n");
#endif
        }
#endif
    }

	printf("SD: found a valid version %s SD card\n", sd_versions[ret->scr->sd_version]);
#ifdef EMMC_DEBUG
	printf("SD: setup successful (status %i)\n", status);
#endif

	// Reset interrupt register
	mmio_write(emmc_base + EMMC_INTERRUPT, 0xffffffff);

	*dev = (struct block_device *)ret;

	return 0;
}

static int sd_ensure_data_mode(struct emmc_block_dev *edev)
{
	if(edev->card_rca == 0)
	{
		// Try again to initialise the card
		int ret = sd_card_init((struct block_device **)&edev);
		if(ret != 0)
			return ret;
	}

#ifdef EMMC_DEBUG
	printf("SD: ensure_data_mode() obtaining status register for card_rca %08x: ",
		edev->card_rca);
#endif

    sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16, 500000);
    if(FAIL(edev))
    {
        printf("SD: ensure_data_mode() error sending CMD13\n");
        edev->card_rca = 0;
        return -1;
    }

	u32 status = edev->last_r0;
	u32 cur_state = (status >> 9) & 0xf;
#ifdef EMMC_DEBUG
	printf("status %i\n", cur_state);
#endif
	if(cur_state == 3)
	{
		// Currently in the stand-by state - select it
		sd_issue_command(edev, SELECT_CARD, edev->card_rca << 16, 500000);
		if(FAIL(edev))
		{
			printf("SD: ensure_data_mode() no response from CMD17\n");
			edev->card_rca = 0;
			return -1;
		}
	}
	else if(cur_state == 5)
	{
		// In the data transfer state - cancel the transmission
		sd_issue_command(edev, STOP_TRANSMISSION, 0, 500000);
		if(FAIL(edev))
		{
			printf("SD: ensure_data_mode() no response from CMD12\n");
			edev->card_rca = 0;
			return -1;
		}

		// Reset the data circuit
		sd_reset_dat();
	}
	else if(cur_state != 4)
	{
		// Not in the transfer state - re-initialise
		int ret = sd_card_init((struct block_device **)&edev);
		if(ret != 0)
			return ret;
	}

	// Check again that we're now in the correct mode
	if(cur_state != 4)
	{
#ifdef EMMC_DEBUG
		printf("SD: ensure_data_mode() rechecking status: ");
#endif
        sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16, 500000);
        if(FAIL(edev))
		{
			printf("SD: ensure_data_mode() no response from CMD13\n");
			edev->card_rca = 0;
			return -1;
		}
		status = edev->last_r0;
		cur_state = (status >> 9) & 0xf;

#ifdef EMMC_DEBUG
		printf("%i\n", cur_state);
#endif

		if(cur_state != 4)
		{
			printf("SD: unable to initialise SD card to "
					"data mode (state %i)\n", cur_state);
			edev->card_rca = 0;
			return -1;
		}
	}

	return 0;
}

#ifdef SDMA_SUPPORT
// We only support DMA transfers to buffers aligned on a 4 kiB boundary
static int sd_suitable_for_dma(void *buf)
{
    if((uintptr_t)buf & 0xfff)
        return 0;
    else
        return 1;
}
#endif

static int sd_do_data_command(struct emmc_block_dev *edev, int is_write, uint8_t *buf, size_t buf_size, u32 block_no)
{
	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!edev->card_supports_sdhc)
		block_no *= 512;

	// This is as per HCSS 3.7.2.1
	if(buf_size < edev->block_size)
	{
	    printf("SD: do_data_command() called with buffer size (%i) less than "
            "block size (%i)\n", buf_size, edev->block_size);
        return -1;
	}

	edev->blocks_to_transfer = buf_size / edev->block_size;
	if(buf_size % edev->block_size)
	{
	    printf("SD: do_data_command() called with buffer size (%i) not an "
            "exact multiple of block size (%i)\n", buf_size, edev->block_size);
        return -1;
	}
	edev->buf = buf;

	// Decide on the command to use
	int command;
	if(is_write)
	{
	    if(edev->blocks_to_transfer > 1)
            command = WRITE_MULTIPLE_BLOCK;
        else
            command = WRITE_BLOCK;
	}
	else
    {
        if(edev->blocks_to_transfer > 1)
            command = READ_MULTIPLE_BLOCK;
        else
            command = READ_SINGLE_BLOCK;
    }

	int retry_count = 0;
	int max_retries = 3;
	while(retry_count < max_retries)
	{
#ifdef SDMA_SUPPORT
	    // use SDMA for the first try only
	    if((retry_count == 0) && sd_suitable_for_dma(buf))
            edev->use_sdma = 1;
        else
        {
#ifdef EMMC_DEBUG
            printf("SD: retrying without SDMA\n");
#endif
            edev->use_sdma = 0;
        }
#else
        edev->use_sdma = 0;
#endif

        sd_issue_command(edev, command, block_no, 5000000);

        if(SUCCESS(edev))
            break;
        else
        {
            printf("SD: error sending CMD%i, ", command);
            printf("error = %08x.  ", edev->last_error);
            retry_count++;
            if(retry_count < max_retries)
                printf("Retrying...\n");
            else
                printf("Giving up.\n");
        }
	}
	if(retry_count == max_retries)
    {
        edev->card_rca = 0;
        return -1;
    }

    return 0;
}

int sd_read(struct block_device *dev, uint8_t *buf, size_t buf_size, u32 block_no)
{
	// Check the status of the card
	struct emmc_block_dev *edev = (struct emmc_block_dev *)dev;
    if(sd_ensure_data_mode(edev) != 0)
        return -1;

#ifdef EMMC_DEBUG
	printf("SD: read() card ready, reading from block %u\n", block_no);
#endif

    if(sd_do_data_command(edev, 0, buf, buf_size, block_no) < 0)
        return -1;

#ifdef EMMC_DEBUG
	printf("SD: data read successful\n");
#endif

	return buf_size;
}

#ifdef SD_WRITE_SUPPORT
int sd_write(struct block_device *dev, uint8_t *buf, size_t buf_size, u32 block_no)
{
	// Check the status of the card
	struct emmc_block_dev *edev = (struct emmc_block_dev *)dev;
    if(sd_ensure_data_mode(edev) != 0)
        return -1;

#ifdef EMMC_DEBUG
	printf("SD: write() card ready, reading from block %u\n", block_no);
#endif

    if(sd_do_data_command(edev, 1, buf, buf_size, block_no) < 0)
        return -1;

#ifdef EMMC_DEBUG
	printf("SD: write read successful\n");
#endif

	return buf_size;
}
#endif

