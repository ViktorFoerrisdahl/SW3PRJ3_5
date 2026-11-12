import sys
import os
import select
import tornado.ioloop

import matplotlib.pyplot as plt

import matplotlib
matplotlib.use("webagg")
matplotlib.rcParams["webagg.port"] = 8988

def stop_ioloop():
    tornado.ioloop.IOLoop.current().stop() # stops webagg server

def shutdown_webagg_server():
    global shutting_down
    if shutting_down:
        return
    shutting_down = True
    axis.clear() # clears whole fig
    #redraw labels and limits
    axis.set_ylabel("Meter")
    axis.set_xlabel("Meter")
    axis.set_xlim(min(mic_x_array) - 1.5, max(mic_x_array) + 1.5)
    axis.set_ylim(min(mic_y_array) - 1.5, max(mic_y_array) + 1.5)
    timer.stop() # stop timer that updates plot
    fig.canvas.draw() # draw new fig with only label and limits
    shutdown_timer.start() #start shutdown timer

# gets filedescriptor from sys which is used by select and os.read
stdin_fd = sys.stdin.fileno() 
# Creates input buffer to store incoming messages from stdin
input_buffer = b""        # saves half messages if they are read in the tick

def poll_stdin():
    # ready is a list of filedescriptors, that has data that is ready to be read
    # select makes reading the stdin from sys non-blocking
    ready, _, _ = select.select([stdin_fd], [], [], 0)  # timeout 0 = non blocking
    if not ready: #if data is not ready. return None
        return None
    return os.read(stdin_fd, 4096)   # else return messages from terminal using file descriptor

def pipe_reciver():
    global message_recived_from_pipe, input_buffer #makes both global and ready to use in function

    stdin_input = poll_stdin() #updates stdin_input buffer with mesasages from stdin

    #checks if input has new message, empty message or if cpp program is closed
    if stdin_input is None:
        pass

    elif stdin_input == b"": #cpp has closed
        shutdown_webagg_server()

    else:
        input_buffer += stdin_input # new message

    while b"\n" in input_buffer: # while we have a new message - indicated with \n
        line, input_buffer = input_buffer.split(b"\n", 1) #split new message into their own line
        message_argv = line.decode(errors="replace").split() #split only the first line into their own arguments and save in message argv

        if not message_argv: # if message_argv is empty - continue. Whitespaces message was recieved
            continue
        try:
            if message_argv[0] == "NEW": # NEW means new datapoint. Add data to pipe_to_plot_array
                new_data = []
                new_data.append(message_argv[0]) # NEW
                new_data.append(float(message_argv[1])) # x value
                new_data.append(float(message_argv[2])) # y value
                new_data.append(int(message_argv[3])) # event ID
                pipe_to_plot_array.append(new_data) #append new data to pipe to plot array
                message_recived_from_pipe = True

            elif message_argv[0] == "DEL": #DEL means delete datapoint. Pops the datapoints array at the front to create FIFO.
                new_data = []
                new_data.append(message_argv[0]) # DEL
                pipe_to_plot_array.append(new_data) #append new data to pipe to plot array
                message_recived_from_pipe = True

            elif message_argv[0] == "MICS":
                new_data = []
                new_data.append(message_argv[0]) # MIC means that new mic positions is recieved
                new_data.append(message_argv[1]) # Amount of mics
                for mic in range(int(new_data[1])):
                    i = 2 + mic * 2
                    new_data.append(message_argv[i])      # x position of mic
                    new_data.append(message_argv[i + 1])  # y position of mic
                pipe_to_plot_array.append(new_data) #append new data to pipe to plot array
                message_recived_from_pipe = True

            elif message_argv[0] == "STOP":
                shutdown_webagg_server()

        except (ValueError, IndexError):
            print(f" error in message: {line!r}", file=sys.stderr)

def update_plot():

    pipe_reciver()

    global message_recived_from_pipe
    if message_recived_from_pipe: # If we recieved a new message:
        while pipe_to_plot_array: # if not-empty = place data from pipe into data_arrays or mic arrays
            new_data = pipe_to_plot_array.pop(0) #place new data into new arrays
            if new_data[0] == "NEW": # NEW = new event data
                data_x_array.append(new_data[1])
                data_y_array.append(new_data[2])
                data_event_id.append(new_data[3])

            elif new_data[0] == "DEL":
                if data_x_array:
                    data_event_id.pop(0)
                    data_x_array.pop(0)
                    data_y_array.pop(0)

            elif new_data[0] == "MICS":
                # Clears old mic arrays and datapoints as they are obsolete
                mic_x_array.clear() 
                mic_y_array.clear()
                micName_array.clear()
                if data_x_array:
                    data_event_id.clear()
                    data_x_array.clear()
                    data_y_array.clear()
                for mic in range(int(new_data[1])):
                    mic_x_array.append(float(new_data[2 + mic * 2]))
                    mic_y_array.append(float(new_data[3 + mic * 2]))
                    micName_array.append(f"mic {mic + 1}")

        #Clears everything drawn on the axis. 
        #This is done to wipe the board clean everytime we update to NOT draw the same things twice on top of each other
        axis.clear()

        # Draw axis parameters again after they have been cleared
        axis.set_ylabel("Meter")
        axis.set_xlabel("Meter")
        axis.set_xlim(min(mic_x_array) - 1.5, max(mic_x_array) + 1.5)
        axis.set_ylim(min(mic_y_array) - 1.5, max(mic_y_array) + 1.5)

        #Plot events as red x
        axis.plot(data_x_array,data_y_array, 'rx')

        #Plot event ID's and coordinates next to events coordinates
        for x, y, event_id in zip(data_x_array, data_y_array, data_event_id):
            text = f"EventID: {event_id} - ({x:.2f}, {y:.2f})"
            axis.annotate(text, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)

        #Plot mics as red dots
        axis.plot(mic_x_array,mic_y_array,'ro')

        #Plot mic names
        for x, y, name in zip(mic_x_array, mic_y_array, micName_array):
            axis.annotate(name, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)

        message_recived_from_pipe = False
        fig.canvas.draw_idle() # redraw canves when new message is received when the event loop got time for it.


if __name__ == "__main__":

    fig, axis = plt.subplots() # Make figure

    message_recived_from_pipe = True # new message is reveived when program is opened for the first time

    shutting_down = False # Flag for shutting down as shut_down_webagg server can be called two times. This flag avoids it.

    amountOfMics = int(sys.argv[1]) # save amount of mic from syscall, as plot is made from syscall at the start.

    #array to new data from pipe to plot
    pipe_to_plot_array = []

    # arrays for events
    data_x_array = []
    data_y_array = [] 
    data_event_id = []

    # arrays for mic locations
    mic_x_array = []
    mic_y_array = []
    micName_array = []

    # input mics info from system argv first time program starts
    for mic in range(amountOfMics):
        mic_x_array.append(float(sys.argv[2 + mic * 2]))
        mic_y_array.append(float(sys.argv[3 + mic * 2]))
        micName_array.append(f"mic {mic + 1}")

    # start timer to use to run update plot at desired interval
    timer = fig.canvas.new_timer(interval=200)
    # add callback function to execute every interval
    timer.add_callback(update_plot)
    timer.start()

    shutdown_timer = fig.canvas.new_timer(interval=1000)
    shutdown_timer.single_shot = True # only makes callback once
    shutdown_timer.add_callback(stop_ioloop) # stops server

    # blocking call to show open web and show fig. Now webplot function gets called every 200 ms to update plot.
    plt.show() 

# https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.pause.html
# https://www.geeksforgeeks.org/python/matplotlib-pyplot-pause-in-python/
# https://stackoverflow.com/questions/2905965/creating-threads-in-python
# https://www.w3schools.com/python/python_dsa_queues.asp
# https://matplotlib.org/stable/api/_as_gen/matplotlib.animation.FuncAnimation.html
# https://stackoverflow.com/questions/190010/daemon-threads-explanation
# https://docs.python.org/3/library/select.html
