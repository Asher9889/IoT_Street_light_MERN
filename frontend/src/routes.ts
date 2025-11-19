import { Home, FileText, Plus } from "lucide-react";
// import Dashboard from "./pages/Dashboard";
import Dashboard from "./pages/dashboard";
// import { AllBlogs, CreateBlog, Blog, EditBlog  } from "./pages";

// export const lazyPage = (path: string) =>
//   lazy(() => import(`../pages/${path}`).then((m) => ({ default: m.default })));

export const navItems = [
  {
    name: "Dashboard",
    path: "/",
    icon: Home,
    element: Dashboard, //Dashboard,
    skip: false,
  },

  {
    name: "Master",
    path: "/master",
    icon: Home,
    element: Dashboard,
    skip: false,
    children: [
      {
        name: "Zone",
        path: "/zone",
        icon: Home,
        element: Dashboard,
        skip: false,
      },
      {
        name: "ward",
        path: "/ward",
        icon: Home,
        element: Dashboard,
        skip: false,
      },
      {
        name: "Area",
        path: "/area",
        icon: Home,
        element: Dashboard,
        skip: false,
      },
      {
        name: "state",
        path: "/state",
        icon: Home,
        element: Dashboard,
        skip: false,
      },
      {
        name: "city",
        path: "/city",
        icon: Home,
        element: Dashboard,
        skip: false,
      },
    ],
  },

  {
    name: "Blogs",
    path: "/blogs",
    icon: FileText,
    element: Dashboard, //AllBlogs,
    skip: false,
    children: [
      // { name: "All Blogs", path: "/blogs", element: NotFound,},
      {
        name: "Create Blog",
        path: "/blogs/create",
        icon: Plus,
        element: Dashboard,
        skip: false,
      },
      // { name: "Edit Blog", path: "/blogs/:id/edit", element: NotFound, },
      { name: "View Blog", path: "/blogs/:slug", element: Dashboard },
      { name: "Edit Blog", path: "/blogs/update/:slug", element: Dashboard },
    ],
  },
];

export const routes = navItems.flatMap((item) => {
  const parent = item.path
    ? [
        {
          name: item.name,
          path: item.path,
          icon: item.icon,
          element: item.element,
          skip: item.skip,
        },
      ]
    : [];

  const children =
    item.children?.map((child) => ({
      name: child.name,
      path: child.path,
      icon: child.icon,
      element: child.element,
      skip: child.skip,
    })) ?? [];

  return [...parent, ...children];
});
