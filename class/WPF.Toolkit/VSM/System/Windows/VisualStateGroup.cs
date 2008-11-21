﻿// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using System.Windows.Media.Animation;

using Mono;

namespace System.Windows
{
    /// <summary>
    ///     A group of mutually exclusive visual states.
    /// </summary>
    [ContentProperty("States")]
    public class VisualStateGroup : DependencyObject
    {
        /// <summary>
        ///     The Name of the VisualStateGroup.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        /// <summary>
        ///     VisualStates in the group.
        /// </summary>
        public IList States
        {
            get
            {
                if (_states == null)
                {
                    _states = new Collection<VisualState>();
                }

                return _states;
            }
        }

        /// <summary>
        ///     Transitions between VisualStates in the group.
        /// </summary>
        public IList Transitions
        {
            get
            {
                if (_transitions == null)
                {
                    _transitions = new Collection<VisualTransition>();
                }

                return _transitions;
            }
        }

        /// <summary>
        ///     VisualState that is currently applied.
        /// </summary>
        internal VisualState CurrentState
        {
            get; set;
        }

        internal VisualState GetState(string stateName)
        {
            for (int stateIndex = 0; stateIndex < States.Count; ++stateIndex)
            {
                VisualState state = (VisualState)States[stateIndex];
                if (state.Name == stateName)
                {
                    return state;
                }
            }

            return null;
        }

        internal Collection<Storyboard> CurrentStoryboards
        {
            get
            {
                if (_currentStoryboards == null)
                {
                    _currentStoryboards = new Collection<Storyboard>();
                }

                return _currentStoryboards;
            }
        }

        internal void StartNewThenStopOld(FrameworkElement element, params Storyboard[] newStoryboards)
        {
            // Start the new Storyboards
            for (int index = 0; index < newStoryboards.Length; ++index)
            {
                if (newStoryboards[index] == null)
                {
                    continue;
                }

                newStoryboards[index].Begin();

                // Silverlight had an issue where initially, a checked CheckBox would not show the check mark
                // until the second frame. They chose to do a Seek(0) at this point, which this line
                // is supposed to mimic. It does not seem to be equivalent, though, and WPF ends up
                // with some odd animation behavior. I haven't seen the CheckBox issue on WPF, so
                // commenting this out for now.
                // newStoryboards[index].SeekAlignedToLastTick(element, TimeSpan.Zero, TimeSeekOrigin.BeginTime);
            }

            // Stop the old Storyboards
            for (int index = 0; index < CurrentStoryboards.Count; ++index)
            {
                if (CurrentStoryboards[index] == null)
                {
                    continue;
                }

                CurrentStoryboards[index].Stop();
            }

            // Hold on to the running Storyboards
            CurrentStoryboards.Clear();
            for (int index = 0; index < newStoryboards.Length; ++index)
            {
                CurrentStoryboards.Add(newStoryboards[index]);
            }
        }

		internal void RaiseCurrentStateChanging(FrameworkElement element, VisualState oldState, VisualState newState, Control control)
		{
			RaiseCurrentStateChanging (new VisualStateChangedEventArgs(oldState, newState, control));
		}

		internal void RaiseCurrentStateChanged(FrameworkElement element, VisualState oldState, VisualState newState, Control control)
		{
			RaiseCurrentStateChanged(new VisualStateChangedEventArgs(oldState, newState, control));
		}

		internal void RaiseCurrentStateChanging (VisualStateChangedEventArgs e)
		{
			EventHandler<VisualStateChangedEventArgs> h = (EventHandler<VisualStateChangedEventArgs>) events[CurrentStateChangingEvent];
			if (h != null)
				h (this, e);
		}

		internal void RaiseCurrentStateChanged (VisualStateChangedEventArgs e)
		{
			EventHandler<VisualStateChangedEventArgs> h = (EventHandler<VisualStateChangedEventArgs>) events[CurrentStateChangedEvent];
			if (h != null)
				h (this, e);
		}

		static object CurrentStateChangingEvent = new object ();
		static object CurrentStateChangedEvent = new object ();

        /// <summary>
        ///     Raised when transition begins
        /// </summary>
		public event EventHandler<VisualStateChangedEventArgs> CurrentStateChanging {
			add {
				if (events[CurrentStateChangingEvent] == null)
					Events.AddHandler (this, "CurrentStateChanging", Events.current_state_changing);
				events.AddHandler (CurrentStateChangingEvent, value);
			}
			remove {
				events.RemoveHandler (CurrentStateChangingEvent, value);
				if (events[CurrentStateChangingEvent] == null)
					Events.RemoveHandler (this, "CurrentStateChanging", Events.current_state_changing);
			}
		}

        /// <summary>
        ///     Raised when transition ends and new state storyboard begins.
        /// </summary>
		public event EventHandler<VisualStateChangedEventArgs> CurrentStateChanged {
			add {
				if (events[CurrentStateChangedEvent] == null)
					Events.AddHandler (this, "CurrentStateChanged", Events.current_state_changed);
				events.AddHandler (CurrentStateChangedEvent, value);
			}
			remove {
				events.RemoveHandler (CurrentStateChangedEvent, value);
				if (events[CurrentStateChangedEvent] == null)
					Events.RemoveHandler (this, "CurrentStateChanged", Events.current_state_changed);
			}
		}


        private Collection<Storyboard> _currentStoryboards;
        private Collection<VisualState> _states;
        private Collection<VisualTransition> _transitions;
    }
}
