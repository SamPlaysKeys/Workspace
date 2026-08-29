---
title: "How to Plan a Homelab Transcript"
date: 2026-06-25
draft: false
type: Transcription
status: Active
---

The follow transcription is from TailscaleUp 2026, and was presented as part of "Day 0 | Community Day".


## Transcript from the presentation:

Thank you all for coming. I know this is out of the way for a lot of folks, but it's really cool to see so many people showing up to an event talking about what started for a lot of folks as a recommendation on how to set up remote access for a homelab.

My name's Sam. I am a consultant at Red Hat. Primarily with OpenShift and with DevOps projects. That means that most of my day is spent working on OpenShift with large distributed platforms. And a lot of what my approach came to when I was thinking, "You know, hey, what can I contribute to a talk here at Day Zero?" is, you know, Tailscale—what is it actually facilitating for people? Because for a lot of people, it starts as just this early ingress point to security, to actually having things more robust than just a laptop running one application that you want remote access to or that you wanted to share.

Right now, we're seeing this evolution in what people are running on their Tailnets, and what they're building with Tailscale. But I think that one of the things which often gets skipped is the planning phase. I don't know how many of you have done project management work—I know that can be a dirty word—but the first thing you do with a project is you plan the project.

I am of the opinion that planning a project can be more than just the first thing, but it should be something you always do. So, I called the presentation "How to Plan Your Perfect Homelab," but just in case, I also have a more "professional" title: "Iterative Planning in Action".

Much like a lot of folks who subscribe to the Agile methodology, I think everything should be iterative, repeating over and over. And I think this can extend to homelabs as well.

Alex talked about how we've seen this large amount of growth in the self-hosting community. Since I'm always looking for reputable data, I went and looked at the surveys from Self-Hosted. They are a really great platform, and still put out a survey every year. In 2023, they had just shy of 1,900 responses. And these were mostly focused on private stacks, home networks... it was largely on experimenting, learning, practicing, or having a development environment at home—anything in that kind of range.

By 2025, it had more than doubled to 4,000-ish responses, again on what is basically a newsletter that isn't connected in any way to things like r/selfhosted. That's a decent amount of growth in just two years, and a lot of that was enabled by COVID, where we had people who couldn't go out, were at home, and had extra money. And they said, "Hey, maybe I'm tired of Google using my photos to train its AI models," or things like that.

So, we also saw a shift from a homelab focused on automation or focused on development to one focused on privacy, on hosting things like Immich, like Jellyfin—as we've all seen people move from Plex—and moving towards hosting those same services themselves.

Now, given that y'all are here and that most of y'all came to learn about Tailscale or to learn more about specific uses: How many of y'all have something you'd consider a homelab?

*(Almost every hand goes up.)*

Makes sense.

How many have more than five devices in their homelab?

*(About half of the hands stay up.)*

Okay.

How many think it's well-documented enough that someone else could step in and know where to find things?

*(There's a lot more guilty looks than raised hands now.)*

All right, that was like four people. Four and a half... wait, Alex, you don't count! Your homelab is all documented on YouTube!

So, Stack Overflow in their survey in 2024 had the really cool statistic that 84% of developers learn using technical documentation. Yes, "learn" is in quotes and there's an asterisk. "Learn" is in the concept of learning how to do the thing with the thing that isn't working or the thing they're adding. And the asterisk is "when Stack Overflow is down". Love that!

But that's still a large amount of developers that lean on technical documentation for the answers to the questions that they're working with. Even if you're using AI and you say, "Hey, what's going on with this?", that's the first place it's checking.

DX in their State of Developer Experience found a very similar thing. They found that 61% of respondents said that they spend at least a half hour a day looking for answers to technical problems through documentation. So, it's not just an "Oh, we should have this". It's a "No, this is one of the biggest things slowing down even enterprises and software development companies". Which hopefully isn't describing any of your homelabbing. If you're homelabbing and it's this big enterprise environment, it probably should be an enterprise environment. But documentation does matter at home as well. Just like with that question I asked earlier: if you can't explain quickly how your lab works, you won't be able to recover it quickly if something happens.

Now, I know I'm a little bit jaded because I work primarily in a GitOps-driven environment, but I do believe that homelabs should be auditable and repeatable. If you can't explain how your setup works, you definitely can't recover it when something goes down. And there's all those little things of: "Wait, why did I make that decision?" "What was that weird thing?" "What was that setting that I had to enable in Proxmox in order to get Windows 11 VMs to boot properly?" You know, all these little weird things.

And so, I know this is talking about documentation, and I know the talk is about planning—I take the perspective that they're the same thing. After all, documentation that you're making is just planning that you're doing after you've already started working on the thing.

---

When I'm talking about planning, I have a pretty robust view: I break it up into four stages of planning. Again, I warned at the beginning: this is iterative. We have stages.

* **Stage 1:** Deciding what your goal is. What's the one-sentence, aspirational mark of "I want to do this"?


* **Stage 2:** Is it achievable? And I don't mean "Can I build it?" or something like that. I mean to consider the constraints, like the amount of budget you have, the space you have, or whether your significant other will be really unhappy if the way that they're watching the show that they want to watch suddenly stops working at 10 PM. These are things you need to consider when you're looking at what you're going to deploy or what you're going to upgrade.


* **Stage 3:** How is it going to work? This is the fun bit where you either actually make a plan and start to build out what that looks like, or possibly you *do* build the thing. Because if you're taking an iterative approach, it doesn't have to just be up front. This is where you would make the scoped plan either physically or written out. And this is also when you do things like create runbooks or ADRs, which are Architectural Decision Records—aka why you did that weird thing.


* **Stage 4:** Does the plan still fit? This is the really fun bit where you get to identify drift, where you say, "Hey, my original goal with a fresh brain was to do this one thing. And I've got all this stuff built and it's really cool. I found this cool solution. I've got it deployed now using Docker Compose. But hold on... wait, does it actually accomplish my goal?" And so this is where you'll tweak that and say, "Oh, maybe I need to adjust my goal a little bit?" And now you have something a little bit more specific that you can take going forward.


---

Originally when I wrote this talk, I was going to crowdsource for an idea, and practice going through the iterative planning together. I don't know why, but I thought I had 40 minutes for the talk. Instead, I have 20, and that is definitely not enough time to do everything by committee. So we're going to pretend that we came up with this goal together!

> "I want Git to be the source of truth for every containerized service I run. Changes should be reviewable, repeatable, and easy to roll back."
> 

How many of y'all think that sounds like a good goal? So if everything's managed via Git, it makes it really easy to document. But that's a separate thing; that would be my mission statement of, "This is what I want from either my future homelab or from this next upgrade I'm making."

Now that we have our goal, we can look at the constraints and figure out what it is that we can or can't do. For me, I don't want to run Kubernetes at home, so I'll use Compose-compatible definitions with Podman or Docker instead. That will keep the platform lightweight while preserving a Git-driven workflow. What I'm doing here is saying, "I know that in this scenario, I wouldn't want to manage Kubernetes. It'd be a little bit much, and I know that Docker Compose is declarative, so I'll go with that as my approach."

Next I'll say, "How is it going to work?" This is when I would go ahead and make all of the plans, figure out how it all connects together. In this case, my design would be: my container definitions, configuration templates, and documentation will all live in Git. Ansible applies the approved revision to my homelab. Secrets are stored separately and encrypted, and manual changes are either committed or overwritten by the next deployment.

Once I've got all this written down, I'll finish writing out my ADRs, and then I'll say, "All right, let's look back at that mission statement and see: does the plan still fit?"

A really great tool here is a thing called an iteration backlog. This is basically the changes that you make to your plan. So you can do that; you can create new ADRs, set new goals. And I think the savvy among you might have noticed in the overview I put ADRs in step three, and here I'm recommending them in step four. That is because it applies in both cases. It's an Architectural Decision Record—it's *why* did I do the thing? And so, if part of it is saying, "Hey, this goal doesn't quite fit anymore, I'm going to go ahead and write an ADR to explain why I think that," then going forward I can always look back at that and go, "Why did I choose Kubernetes over Docker? What was it?" And then I've got my reasoning right there.

In this case I can say the plan fits because I can deploy, recover, and roll back services through Git without needing to SSH into any machine. However, it's only going to change containers when they're updated in Git, so I might need workflows like Dependabot (a really common one) or compatible management software like Komodo.

For example, if I say I want the Tailscale container and I just use the `latest` tag, with what I described in phase three, it's going to get the latest whenever I deployed it, and then it's never going to update again. It will be on that version forever until I make a change in Git that touches the Tailscale container, and then it will go, "Hey, let's update," and get the new `latest`. But there's nothing actively maintaining it.

Looking at that, I can say "Okay, hold on. That's not quite what I wanted. Time to go back to step one: Goal 2.0."

And so this time I have a little bit more detail. My new goal will be, "I want Git to be the source of truth for every containerized service I run while allowing containers to maintain their own state. Changes should be reviewable and easy to roll back."

Trimming it down a little bit like that is how you begin to get a more specific plan. Now, this won't go on forever. Eventually, you'll reach a solution where you actually meet your goal. But at the end of it, what you'll have is this whole written-out plan of: here's what I want, here's the ways to do it. You might have artifacts; you might have those things already built while building it. In step three, you might say, "Hey, it didn't meet my goal, but now I've got the Compose files, so I'm already good. Those aren't going to change; I'm just going to adjust around it a little bit." And you'll begin to get all the documentation so that, yes, someone else could step into your environment and immediately rebuild or at least understand exactly why you did the weird thing.

Now, I know a lot of folks are talking about AI right now. This is something that I use it a lot for. I don't use AI for code development as much as some other people do, but for me, it is the ideal rubber ducky.

I'm assuming everyone's familiar with the rubber ducky concept. I see a couple of people shaking their heads. Ok, let me quickly explain: There's an idea in computer programming, software development, and troubleshooting where if you need help, you call someone over and say, "Hey, I'm dealing with this issue, can you take a look too?" And as you explain the issue to them, you realize what the problem was. Which means you've wasted that co-worker's time for something that you just needed to explain to someone to figure out what was wrong.

So there was a computer science professor who, when students graduated, gave them all rubber ducks instead. He said "When you need to explain your code to someone when having a problem, explain it to the rubber duck. You'll probably figure out what your issue was, and then you don't waste a co-worker's time."

For me, AI is like the ultimate rubber duck because I can explain things to it, and it's not wasting a co-worker's time. I can do it with a very small local model, but then it also will look at what I have and give a little bit of feedback like, "Hey, you're a doofus, that's a setting you turned broke earlier and said you'd come back to it."

Using AI as a sounding board allows me to ask better questions and find those missed spots in my documentation where I thought I knew everything, but it was only because I was the one who did it.

But what I *won't* do is let AI write those ADRs for me. Because the decision records should be *my* decisions. If AI is making the decisions, it's not my homelab anymore—it's Claude's homelab, and I don't want to be the flesh puppet for Claude!

So while I still make sure anything verification- or decision-based is all me, I can still lean on AI. Google Cloud's DORA (DevOps Research and Assessment) 2024 review found that a 25% increase in AI adoption and usage resulted in a 7.5% increase in the quality of documentation. That doesn't sound like much, but when you think about it, if almost one out of every 10 pieces of documentation was a little bit better, my life would be a lot easier!

While making this talk, I was thinking, "Okay, this is a lot of information, but it's just me recommending a way to make a plan. What can I actually deliver from this?" So as a way to try it out for yourself (or just help bootstrap your planning process) I made two resources.

The one on the left is a homelab documentation kit. It's got a bootstrap file that will just spin up a local environment so you can add it to whatever your current homelab repo is, and it will create the planning phases and places to fill in the information. You can say, "Hey, my homelab's name is Steve's Lab," and it will fill that in where it needs it. It will begin to add the different files that are needed so that either by yourself or working with an AI assistant, you can audit your own stuff and take that same approach.

The one on the right is the same thing, but as a GitHub template. It doesn't have a bootstrap file, but it means you can just apply the template to a new repo and go from there. You can use GitHub Codespaces if you're one of like eight people that I think use it!

These resources are a way to apply that planning process in your own environment without having to review a video or write it all out yourself.

Ok, that is all from me! We have two minutes—does anyone have any questions?

**Audience Member:** On the topic of writing ADRs, my current process is to do a `/grillme`-type engagement with an agent, explain the architectural decisions, and then have it actually write the ADR. Then I review and iterate on the ADR until I'm satisfied. Is that the de facto best practice method, or do you do something else?

**Sam:** Great question! The `/grillme` approach was actually the thing that kicked off me trying to quantify this planning process. I was unhappy with how long it took and how many tokens I burned with `/grillme` to in order to get something functional for what I was doing. I think it's a great agent skill, but expanding it into the full planning process was partially my attempt to get better and more consistent results when reviewing with an agent.

Anybody else?

(This time the room's got nothing—no hands.)

Well, with that, thank you all so much! You can find me on GitHub and Twitter/X at `@samplayaskeys`. Thank you all so much for listening!

