import matplotlib
import sys
import matplotlib.pyplot as plt
import matplotlib.animation as ani
from threading import Thread

matplotlib.use("webagg")
matplotlib.rcParams["webagg.port"] = 8988

def update_plot(placeholder): # Needs placeholder in argument, to be able to be used in ani.funcanimation call, even through it is not used

    if pipe_to_plot_array: # if not-empty = place data from pipe into data_arrays or mic arrays
        new_data = pipe_to_plot_array.pop(0) #place new data into new arrays
        if new_data[0] == "D": # D = new event data
            data_x_array.append(new_data[1])
            data_y_array.append(new_data[2])
            data_event_id.append(new_data[3])

    #Plot events as red x
    axis.plot(data_x_array,data_y_array, 'rx')

    #Plot event ID's and koordinates next to events coordinates
    for x, y, event_id in zip(data_x_array, data_y_array, data_event_id):
        tekst = f"EventID: {event_id}: ({x:.4f}, {y:.4f})"
        axis.annotate(tekst, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)

    #Plot mics as red dots
    axis.plot(mic_x_array,mic_y_array,'ro')

    #Plot mic names
    for x, y, name in zip(mic_x_array, mic_y_array, micName_array):
        axis.annotate(name, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)
    

def pipe_reciver():
    for message in sys.stdin: # read message from terminal. Wait here, if nothing is pasted into terminal
        message = message.strip() # removes whitespaces in front and back
        message_argv = message.split() # message is string - needs to be vector to acces each argument - split makes it into vector
        if not message_argv: # if message_argv is empty - continue. Whitespaces message was recieved
            continue
        elif message_argv[0] == "D": # D means new datapoint. Add data to pipe_to_plot
            new_data = []
            new_data.append(message_argv[0]) # D
            new_data.append(float(message_argv[1])) # x value
            new_data.append(float(message_argv[2])) # y value
            new_data.append(int(message_argv[3])) # event ID
            pipe_to_plot_array.append(new_data)

if __name__ == "__main__":

    fig, axis = plt.subplots() # Make figure

    amountOfMics = int(sys.argv[1]) # save amount of mic from syscall, as plot is made from syscall at the start.

    #array to new data from pipe to plot
    pipe_to_plot_array = []

     # arrays for events
    data_x_array = []
    data_y_array = [] 
    data_event_id = []

    # draw mics from system argv
    mic_x_array = []
    mic_y_array = []
    micName_array = []
    for mic in range(amountOfMics):
        mic_x_array.append(float(sys.argv[2 + mic * 2]))
        mic_y_array.append(float(sys.argv[3 + mic * 2]))
        micName_array.append(f"mic {mic + 1}")

    axis.set_ylabel("Meter")
    axis.set_xlabel("Meter")
    axis.set_xlim(min(mic_x_array) - 1, max(mic_x_array) + 1)
    axis.set_ylim(min(mic_y_array) - 1, max(mic_y_array) + 1)

    # start thread to control pipe reciever
    pipe_thread = Thread(target=pipe_reciver,daemon=True) # made daemon to not need join. If main program is stopped, so is this thread.
    pipe_thread.start()   

    # start server with desired fig, function to be called every 200 ms to update fig 
    # cache frama data = false, as we dont want to cache the frames as it goes on forever
    webplot = ani.FuncAnimation(fig,update_plot,interval=200,cache_frame_data=False,) 

    # blocking call to show open web and show fig. Now webplot function gets called every 200 ms to update plot.
    plt.show() 

# https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.pause.html
# https://www.geeksforgeeks.org/python/matplotlib-pyplot-pause-in-python/
# https://stackoverflow.com/questions/2905965/creating-threads-in-python
# https://www.w3schools.com/python/python_dsa_queues.asp
# https://matplotlib.org/stable/api/_as_gen/matplotlib.animation.FuncAnimation.html
# https://stackoverflow.com/questions/190010/daemon-threads-explanation