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


def plot_camera_image(
    pix_x: np.ndarray,
    pix_y: np.ndarray,
    pix_size: float,
    pe: np.ndarray,
    mask: np.ndarray = None,
    vmin: float = None,
    vmax: float = None,
    cut_radius: float = None,
    title: str = None,
    pixel_shape: str = "square",  # new parameter, 'square' or 'hex'
) -> plt.Axes:
    """
    Plot the camera image with square or hexagonal pixels.

    Parameters
    ----------
    pix_x : np.ndarray
        X coordinates of pixel centers
    pix_y : np.ndarray
        Y coordinates of pixel centers
    pix_size : float
        Side length of square pixels or flat-to-flat distance of hex pixels
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
    pixel_shape : str, optional
        Shape of pixel, either "square" or "hex". (default: "square")

    Returns
    -------
    plt.Axes
        The axes object with the camera image
    """
    from matplotlib.patches import RegularPolygon

    fig, ax = plt.subplots(figsize=(8, 8))

    if mask is None:
        mask = np.ones(len(pix_x), dtype=bool)

    if vmin is None:
        vmin = np.min(pe[mask]) if np.any(mask) else 0
    if vmax is None:
        vmax = np.max(pe[mask]) if np.any(mask) else 1

    pix_r = np.sqrt(pix_x ** 2 + pix_y ** 2)
    distance_flag = None
    if cut_radius is not None:
        distance_flag = pix_r < cut_radius

    cmap = plt.cm.viridis
    norm = mcolors.Normalize(vmin=vmin, vmax=vmax)

    for i in range(len(pix_x)):
        x_center = pix_y[i]
        y_center = pix_x[i]

        if mask[i]:
            color = cmap(norm(pe[i]))
            facecolor = color
        else:
            facecolor = 'lightgray'

        if pixel_shape.lower() == "hex":
            # Use RegularPolygon to draw hexagon
            # The given pix_size is flat-to-flat distance, which is 2*radius
            hex_radius = pix_size / np.sqrt(3)
            # RegularPolygon needs orientation to make horizontal flat-to-flat
            hex_patch = RegularPolygon(
                (x_center, y_center), numVertices=6, radius=hex_radius,
                orientation=np.radians(30),  # 0 degrees makes a flat top
                facecolor=facecolor, edgecolor='black', linewidth=0.1
            )
            shape_patch = hex_patch
        else:
            # Default: square
            bottom_left_x = x_center - pix_size / 2
            bottom_left_y = y_center - pix_size / 2
            shape_patch = Rectangle(
                (bottom_left_x, bottom_left_y),
                pix_size,
                pix_size,
                facecolor=facecolor,
                edgecolor='black',
                linewidth=0.1
            )

        if distance_flag is not None:
            if distance_flag[i]:
                ax.add_patch(shape_patch)
        else:
            ax.add_patch(shape_patch)

    ax.set_aspect('equal')
    ax.set_xlabel('Y [deg]')
    ax.set_ylabel('X [deg]')
    if title is not None:
        ax.set_title(title)
    else:
        ax.set_title('Camera Image')

    x_margin = pix_size
    y_margin = pix_size
    ax.set_xlim(np.min(pix_y) - y_margin, np.max(pix_y) + y_margin)
    ax.set_ylim(np.min(pix_x) - x_margin, np.max(pix_x) + x_margin)

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


def plot_event(pix_x:np.ndarray, pix_y:np.ndarray, pix_size:float, event, plot_true: bool, pix_shape: str = "square", use_fake_image: bool = False, title=None):
    """
    Plot the event.
    """

    from ..helper import convert_to_fov
    true_camera_x, true_camera_y = convert_to_fov(event.simulation.shower.alt, event.simulation.shower.az, event.pointing.array_altitude, event.pointing.array_azimuth)
    if(plot_true):  
        for tel_id, simulated_camera in event.simulation.tels.items():
            if(use_fake_image):
                pe = simulated_camera.fake_image
                mask = simulated_camera.fake_image_mask
            else:
                pe = simulated_camera.true_image
                mask = simulated_camera.true_image != 0
            impact_distance = simulated_camera.impact_parameter
            if(len(pe) == 0 or len(mask) == 0):
                continue
            ax = plot_camera_image(pix_x, pix_y, pix_size, pe, mask, pixel_shape=pix_shape, title=title)
            image_parameter = simulated_camera.image_parameters
            miss = np.degrees(image_parameter.extra.miss)
            label_string = f'Intensity: {np.sum(pe[mask]):.2f} Impact Distance: {impact_distance:.2f} Miss: {miss:.3f}'
            plot_hillas_circle(ax, image_parameter.hillas.x, image_parameter.hillas.y, image_parameter.hillas.length, image_parameter.hillas.width, image_parameter.hillas.psi)
            ax.text(0.05, 0.95, label_string, transform=ax.transAxes, verticalalignment='top', horizontalalignment='left', fontsize=12)
            ax.plot(true_camera_y, true_camera_x, 'r*', markersize=10)
            if(event.dl2 is  None):
                continue
            if("HillasReconstructor" in event.dl2.geometry and event.dl2.geometry["HillasReconstructor"].is_valid):
                rec_camera_x, rec_camera_y = convert_to_fov(event.dl2.geometry["HillasReconstructor"].alt, event.dl2.geometry["HillasReconstructor"].az, event.pointing.array_altitude, event.pointing.array_azimuth)
                ax.plot(rec_camera_y, rec_camera_x, 'g*', markersize=10)
            if("HillasWeightedReconstructor" in event.dl2.geometry and event.dl2.geometry["HillasWeightedReconstructor"].is_valid):
                weighted_rec_camera_x, weighted_rec_camera_y = convert_to_fov(event.dl2.geometry["HillasWeightedReconstructor"].alt, event.dl2.geometry["HillasWeightedReconstructor"].az, event.pointing.array_altitude, event.pointing.array_azimuth)
                ax.plot(weighted_rec_camera_y, weighted_rec_camera_x, 'b*', markersize=10)
            
    else:
        for tel_id, dl1_camera in event.dl1.tels.items():
            pe = dl1_camera.image
            mask = dl1_camera.mask
            ax = plot_camera_image(pix_x, pix_y, pix_size, pe, mask, pixel_shape=pix_shape)
            image_parameter = dl1_camera.image_parameters
            label_string = f'Camera {tel_id} Intensity: {image_parameter.hillas.intensity:.2f} Miss: {np.degrees(image_parameter.extra.miss):.3f}'
            plot_hillas_circle(ax, image_parameter.hillas.x, image_parameter.hillas.y, image_parameter.hillas.length, image_parameter.hillas.width, image_parameter.hillas.psi)
            ax.text(0.05, 0.95, label_string, transform=ax.transAxes, verticalalignment='top', horizontalalignment='left')
            ax.plot(true_camera_y, true_camera_x, 'r*', markersize=10)
    return ax