import { ChevronRight } from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";

import { Sidebar } from "./../ui/sidebar";
import {
  SidebarContent,
  SidebarGroup,
  SidebarGroupContent,
  SidebarGroupLabel,
  SidebarMenu,
  SidebarMenuSub,
  SidebarMenuSubButton,
  SidebarMenuSubItem,
  SidebarMenuButton,
  SidebarMenuItem,
  SidebarHeader,
  SidebarFooter,
} from "@/components/ui/sidebar";
import { Avatar, AvatarImage, AvatarFallback } from "@/components/ui/avatar";

import { navItems } from "@/routes";
import { NavLink } from "react-router-dom";
import React from "react";
import { FaLightbulb } from "react-icons/fa";

export function AppSidebar() {
  const [openMenus, setOpenMenus] = React.useState<Record<string, boolean>>({});
  const toggleMenu = (name: string) => {
    setOpenMenus((prev) => ({
      ...prev,
      [name]: !prev[name],
    }));
  };
  return (
    <Sidebar collapsible="icon">
      <SidebarContent>
        <SidebarHeader>
          <div className="flex items-center gap-2 px-2 py-1">
            <FaLightbulb className="size-6 shrink-0 text-blue-700" />
            {/* This hides ONLY text when collapsed */}
            <span className="group-data-[collapsible=icon]:hidden font-bold">
              PollManagement
            </span>
          </div>
        </SidebarHeader>
        <SidebarGroup>
          <SidebarGroupLabel className="font-bold">Platform</SidebarGroupLabel>
          <SidebarGroupContent>
            <SidebarMenu>
              {navItems
                //@ts-ignore
                .filter((item) => !item.skip)
                .map((item) => {
                  console.log("tems from", item);
                  return (
                    <SidebarItem
                      key={item.name}
                      item={item}
                      pathname={item.path}
                      openMenus={openMenus}
                      toggleMenu={toggleMenu}
                    />
                  );
                })}
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>
      </SidebarContent>
      <SidebarFooter>
        <div className="flex items-center gap-2 px-2 py-2">
          <Avatar>
            <AvatarImage src="https://tse1.mm.bing.net/th/id/OIP.svvjaw8Y6vlNJug9ET2v_AHaHa?pid=Api&P=0&h=180" />
            <AvatarFallback>MR</AvatarFallback>
          </Avatar>

          {/* Hide text when collapsed */}
          <div className="group-data-[collapsible=icon]:hidden">
            <p className="text-sm font-medium">Mohit Rajpoot</p>
            <p className="text-xs text-muted-foreground">Admin</p>
          </div>
        </div>
      </SidebarFooter>
    </Sidebar>
  );
}

// function SidebarItem({ item, pathname, openMenus, toggleMenu }: any) {
//   const isOpen = openMenus[item.name];

//   if (item.children?.length) {
//     return (
//       <SidebarMenuItem>
//         <SidebarMenuButton
//           onClick={() => toggleMenu(item.name)}
//           tooltip={item.name}
//           className="flex w-full items-center justify-between"
//         >
//           {/* LEFT */}
//           <div className="flex items-center gap-2">
//             {item.icon && <item.icon className="size-4" />}
//             <span>{item.name}</span>
//           </div>

//           {/* RIGHT ARROW */}
//           <ChevronRight
//             className={`size-4 transition-transform duration-200 ${
//               isOpen ? "rotate-90" : "rotate-0"
//             }`}
//           />
//         </SidebarMenuButton>

//         {/* ANIMATED CHILDREN */}
//         <div
//           className={`
//             overflow-hidden transition-all duration-300
//             ${isOpen ? "max-h-96 opacity-100" : "max-h-0 opacity-0"}
//           `}
//         >
//           <SidebarMenuSub>
//             {item.children
//               .filter((sub: any) => !sub.skip)
//               .map((sub: any) => (
//                 <SidebarMenuSubItem key={sub.name}>
//                   <SidebarMenuSubButton
//                     asChild
//                     isActive={pathname === sub.path}
//                   >
//                     <NavLink to={sub.path}>
//                       {sub.icon && <sub.icon className="size-4" />}
//                       <span>{sub.name}</span>
//                     </NavLink>
//                   </SidebarMenuSubButton>
//                 </SidebarMenuSubItem>
//               ))}
//           </SidebarMenuSub>
//         </div>
//       </SidebarMenuItem>
//     );
//   }

//   // SIMPLE ITEM
//   return (
//     <SidebarMenuItem>
//       <SidebarMenuButton
//         asChild
//         isActive={pathname === item.path}
//         tooltip={item.name}
//       >
//         <NavLink to={item.path}>
//           {item.icon && <item.icon className="size-4" />}
//           <span>{item.name}</span>
//         </NavLink>
//       </SidebarMenuButton>
//     </SidebarMenuItem>
//   );
// }

function SidebarItem({ item, pathname, openMenus, toggleMenu }: any) {
  // Detect if a child is active
  const hasActiveChild = item.children?.some(
    (sub: any) => pathname === sub.path
  );

  // Auto-open if active child OR manually opened
  const isOpen = hasActiveChild || openMenus[item.name];

  // PARENT WITH CHILDREN
  if (item.children?.length) {
    return (
      <SidebarMenuItem>
        <SidebarMenuButton
          onClick={() => toggleMenu(item.name)}
          tooltip={item.name}
          className={`flex w-full items-center justify-between px-2 py-2 
            ${hasActiveChild ? "bg-muted text-foreground font-medium" : ""}`}
        >
          <div className="flex items-center gap-2">
            {item.icon && <item.icon className="size-4" />}
            <span>{item.name}</span>
          </div>

          <ChevronRight
            className={`size-4 transition-transform duration-200 ${
              isOpen ? "rotate-90" : "rotate-0"
            }`}
          />
        </SidebarMenuButton>

        {/* Children with Framer Motion or normal */}
        <AnimatePresence initial={false}>
          {isOpen && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: "auto", opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              transition={{ duration: 0.25, ease: "easeInOut" }}
            >
              <SidebarMenuSub>
                {item.children.map((sub: any) => (
                  <SidebarMenuSubItem key={sub.name}>
                    <SidebarMenuSubButton
                      asChild
                      isActive={pathname === sub.path}
                    >
                      <NavLink to={sub.path}>
                        {sub.icon && <sub.icon className="size-4" />}
                        <span>{sub.name}</span>
                      </NavLink>
                    </SidebarMenuSubButton>
                  </SidebarMenuSubItem>
                ))}
              </SidebarMenuSub>
            </motion.div>
          )}
        </AnimatePresence>
      </SidebarMenuItem>
    );
  }

  // SIMPLE ITEM
  return (
    <SidebarMenuItem>
      <SidebarMenuButton
        asChild
        isActive={pathname === item.path}
        tooltip={item.name}
      >
        <NavLink to={item.path}>
          {item.icon && <item.icon className="size-4" />}
          <span>{item.name}</span>
        </NavLink>
      </SidebarMenuButton>
    </SidebarMenuItem>
  );
}
