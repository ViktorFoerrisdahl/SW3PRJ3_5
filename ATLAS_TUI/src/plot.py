import matplotlib
import sys
import matplotlib.pyplot as plt
import matplotlib.animation as ani
from threading import Thread

matplotlib.use("webagg")
matplotlib.rcParams["webagg.port"] = 8988


def update_plot(placeholder):

    if pipe_to_plot_array:
        new_data = pipe_to_plot_array.pop(0)
        data_x_array.append(new_data[0])
        data_y_array.append(new_data[1])
        data_event_id.append(new_data[2])

    #Plot events  
    axis.plot(data_x_array,data_y_array, 'rx')

    #Plot event ID's next to events coordinates
    for x, y, name in zip(data_x_array, data_y_array, data_event_id):
        axis.annotate(name, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)

    #Plot mics
    axis.plot(mic_x_array,mic_y_array,'ro')

    #Plot mic names
    for x, y, name in zip(mic_x_array, mic_y_array, micName_array):
        axis.annotate(name, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)
    

def pipe_reciver():
    for message in sys.stdin:
        message = message.strip() #removes whitespaces in front and back
        message_argv = message.split() # message is string - needs to be vector to acces each argument
        if not message_argv:
            continue
        elif message_argv[0] == "N":
            new_data = []
            new_data.append(float(message_argv[1]))
            new_data.append(float(message_argv[2]))
            new_data.append(int(message_argv[3]))
            pipe_to_plot_array.append(new_data)

if __name__ == "__main__":

    fig, axis = plt.subplots()

    amountOfMics = int(sys.argv[1])

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
    axis.set_xlim(min(mic_x_array) - 2, max(mic_x_array) + 2)
    axis.set_ylim(min(mic_y_array) - 2, max(mic_y_array) + 2)

    pipe_thread = Thread(target=pipe_reciver,daemon=True)
    pipe_thread.start()   

    webplot = ani.FuncAnimation(fig,update_plot,200,cache_frame_data=False)

    plt.show()

# https://matplotlib.org/stable/api/_as_gen/matplotlib.pyplot.pause.html
# https://www.geeksforgeeks.org/python/matplotlib-pyplot-pause-in-python/
# https://stackoverflow.com/questions/2905965/creating-threads-in-python
# https://www.w3schools.com/python/python_dsa_queues.asp