"""
Written by:
    @author: Zhipeng Zhang
    @email: 
    @date: 2025-10-12
"""
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.patches import Circle, Rectangle, Ellipse
from mpl_toolkits.axes_grid1 import make_axes_locatable
from typing import Dict, List, Optional, Tuple, Union
from dataclasses import dataclass
import sys


def plot_camera_image(pix_x:np.ndarray, pix_y: np.ndarray, pix_size:float, pe: np.ndarray, mask: np.ndarray = None, vmin: float = None, vmax: float = None, cut_radius: float = None, title: str = None) -> plt.Axes:
    """
    Plot the camera image with square pixels.
    
    Parameters
    ----------
    pix_x : np.ndarray
        X coordinates of pixel centers
    pix_y : np.ndarray
        Y coordinates of pixel centers
    pix_size : float
        Side length of square pixels
    pe : np.ndarray
        Photoelectron values for each pixel
    mask : np.ndarray, optional
        Boolean mask indicating which pixels to show with pe values.
        If None, all pixels show pe values.
    vmin : float, optional
        Minimum value for color scale
    vmax : float, optional
        Maximum value for color scale
    cut_radius : float, optional
        Cut radius for the camera image
    title : str, optional
        Title of the camera image
        
    Returns
    -------
    plt.Axes
        The axes object with the camera image
    """
    fig, ax = plt.subplots(figsize=(8, 8))
    
    # If no mask is provided, use all pixels
    if mask is None:
        mask = np.ones(len(pix_x), dtype=bool)
    
    # Determine color scale limits
    if vmin is None:
        vmin = np.min(pe[mask]) if np.any(mask) else 0
    if vmax is None:
        vmax = np.max(pe[mask]) if np.any(mask) else 1
    
    pix_r = np.sqrt(pix_x**2 + pix_y**2)
    distance_flag = None
    if cut_radius is not None:
        distance_flag = pix_r < cut_radius
    # Create colormap
    cmap = plt.cm.viridis
    norm = mcolors.Normalize(vmin=vmin, vmax=vmax)
    
    # Draw all pixels (swap x and y: x->vertical, y->horizontal)
    for i in range(len(pix_x)):
        # Calculate bottom-left corner from center position
        # Swap: pix_y becomes horizontal (matplotlib x), pix_x becomes vertical (matplotlib y)
        bottom_left_x = pix_y[i] - pix_size / 2
        bottom_left_y = pix_x[i] - pix_size / 2
        
        if mask[i]:
            # Masked pixels: show with pe value
            color = cmap(norm(pe[i]))
            rect = Rectangle((bottom_left_x, bottom_left_y), pix_size, pix_size,
                           facecolor=color, edgecolor='black', linewidth=0.1)
        else:
            # Unmasked pixels: show as empty (white or light gray)
            rect = Rectangle((bottom_left_x, bottom_left_y), pix_size, pix_size,
                           facecolor='lightgray', edgecolor='black', linewidth=0.1)
        
        if(distance_flag is not None ):
            if(distance_flag[i]):
                ax.add_patch(rect)
        else:
            ax.add_patch(rect)
    # Set equal aspect ratio and tight layout
    ax.set_aspect('equal')
    ax.set_xlabel('Y [deg]')  # Y is now horizontal
    ax.set_ylabel('X [deg]')  # X is now vertical
    if(title is not None):
        ax.set_title(title)
    else:
        ax.set_title('Camera Image')
    
    # Set axis limits with some padding (swap x and y)
    x_margin = pix_size
    y_margin = pix_size
    ax.set_xlim(np.min(pix_y) - y_margin, np.max(pix_y) + y_margin)
    ax.set_ylim(np.min(pix_x) - x_margin, np.max(pix_x) + x_margin)
    
    # Add colorbar
    divider = make_axes_locatable(ax)
    cax = divider.append_axes("right", size="5%", pad=0.1)
    sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
    sm.set_array([])
    plt.colorbar(sm, cax=cax, label='Photoelectrons')
    
    return ax

def plot_hillas_circle(ax: plt.Axes, hillas_x:float, hillas_y:float, hillas_length:float, hillas_width:float, hillas_psi:float) -> plt.Axes:
    """
    Plot the hillas ellipse (swap x and y: x->vertical, y->horizontal).
    """
    hillas_x_deg = np.degrees(hillas_x)
    hillas_y_deg = np.degrees(hillas_y)
    hillas_length_deg = np.degrees(hillas_length) * 2
    hillas_width_deg = np.degrees(hillas_width) * 2
    hillas_psi_deg = np.degrees(hillas_psi)
    # Swap x and y: hillas_y becomes horizontal (matplotlib x), hillas_x becomes vertical (matplotlib y)
    # Also swap width and height, and adjust angle by 90 degrees
    circle = Ellipse(xy=(hillas_y_deg, hillas_x_deg), width=hillas_length_deg, height=hillas_width_deg, angle=90 - hillas_psi_deg , fill=False, color='red', linewidth=2)
    ax.add_patch(circle)
    return ax


def plot_event(pix_x:np.ndarray, pix_y:np.ndarray, pix_size:float, event, plot_true: bool):
    """
    Plot the event.
    """

    from ..helper import convert_to_fov
    true_camera_x, true_camera_y = convert_to_fov(event.simulation.shower.alt, event.simulation.shower.az, event.pointing.array_altitude, event.pointing.array_azimuth)
    if(plot_true):  
        for tel_id, simulated_camera in event.simulation.tels.items():
            pe = simulated_camera.fake_image
            mask = simulated_camera.fake_image_mask
            if(len(pe) == 0 or len(mask) == 0):
                continue
            ax = plot_camera_image(pix_x, pix_y, pix_size, pe, mask)
            image_parameter = simulated_camera.image_parameters
            label_string = f'Camera {tel_id} Intensity: {image_parameter.hillas.intensity:.2f} Miss: {np.degrees(image_parameter.extra.miss):.3f}'
            plot_hillas_circle(ax, image_parameter.hillas.x, image_parameter.hillas.y, image_parameter.hillas.length, image_parameter.hillas.width, image_parameter.hillas.psi)
            ax.text(0.05, 0.95, label_string, transform=ax.transAxes, verticalalignment='top', horizontalalignment='left')
            ax.plot(true_camera_y, true_camera_x, 'r*', markersize=10)
            if(event.dl2.geometry["HillasReconstructor"].is_valid):
                rec_camera_x, rec_camera_y = convert_to_fov(event.dl2.geometry["HillasReconstructor"].alt, event.dl2.geometry["HillasReconstructor"].az, event.pointing.array_altitude, event.pointing.array_azimuth)
                ax.plot(rec_camera_y, rec_camera_x, 'g*', markersize=10)
            if(event.dl2.geometry["HillasWeightedReconstructor"].is_valid):
                weighted_rec_camera_x, weighted_rec_camera_y = convert_to_fov(event.dl2.geometry["HillasWeightedReconstructor"].alt, event.dl2.geometry["HillasWeightedReconstructor"].az, event.pointing.array_altitude, event.pointing.array_azimuth)
                ax.plot(weighted_rec_camera_y, weighted_rec_camera_x, 'b*', markersize=10)
            
    else:
        for tel_id, dl1_camera in event.dl1.tels.items():
            pe = dl1_camera.image
            mask = dl1_camera.mask
            ax = plot_camera_image(pix_x, pix_y, pix_size, pe, mask)
            image_parameter = dl1_camera.image_parameters
            label_string = f'Camera {tel_id} Intensity: {image_parameter.hillas.intensity:.2f} Miss: {np.degrees(image_parameter.extra.miss):.3f}'
            plot_hillas_circle(ax, image_parameter.hillas.x, image_parameter.hillas.y, image_parameter.hillas.length, image_parameter.hillas.width, image_parameter.hillas.psi)
            ax.text(0.05, 0.95, label_string, transform=ax.transAxes, verticalalignment='top', horizontalalignment='left')
            ax.plot(true_camera_y, true_camera_x, 'r*', markersize=10)
    return ax